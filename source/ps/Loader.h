/* Copyright (C) 2025 Wildfire Games.
 * This file is part of 0 A.D.
 *
 * 0 A.D. is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * 0 A.D. is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with 0 A.D.  If not, see <http://www.gnu.org/licenses/>.
 */

// FIFO queue of load 'functors' with time limit; enables displaying
// load progress without resorting to threads (complicated).

#ifndef INCLUDED_LOADER
#define INCLUDED_LOADER

#include "lib/debug.h"
#include "lib/status.h"

#include <functional>
#include <string>

namespace PS::Loader
{
/*

[KEEP IN SYNC WITH WIKI!]

Overview
--------

"Loading" is the act of preparing a game session, including reading all
required data from disk. Ideally, this would be very quick, but for complex
maps and/or low-end machines, a duration of several seconds can be expected.
Having the game freeze that long is unacceptable; instead, we want to display
the current progress and task, which greatly increases user patience.


Allowing for Display
--------------------

To display progress, we need to periodically 'interrupt' loading.
Threads come to mind, but there is a problem: since OpenGL graphics calls
must come from the main thread, loading would have to happen in a
background thread. Everything would thus need to be made thread-safe,
which is a considerable complication.

Therefore, we load from a single thread, and split the operation up into
"tasks" (as short as possible). These are typically function calls instead of
being called directly, they are registered with our queue. We are called from
the main loop and process as many tasks as possible within one "timeslice".

After that, progress is updated: an estimated duration for each task
(derived from timings on one machine) is used to calculate headway.
As long as task lengths only differ by a constant factor between machines,
this timing is exact; even if not, only smoothness of update suffers.


Interrupting Lengthy Tasks
--------------------------

The above is sufficient for basic needs, but breaks down if tasks are long
(> 500ms). To fix this, we will need to modify the tasks themselves:
either make them coroutines, i.e. have them return to the main loop and then
resume where they left off, or re-enter a limited version of the main loop.
The former requires persistent state and careful implementation,
but yields significant advantages:
- progress calculation is easy and smooth,
- all services of the main loop (especially input*) are available, and
- complexity due to reentering the main loop is avoided.

* input is important, since we want to be able to abort long loads or
even exit the game immediately.

We therefore go with the 'coroutine' (more correctly 'generator') approach.
Examples of tasks that take so long and typical implementations may
be seen in MapReader.cpp.


Intended Use
------------

  PS::Loader::BeginRegistering();
  PS::Loader::Register(..) for each sub-function
  PS::Loader::EndRegistering();
Then in the main loop, call PS::Loader::ProgressiveLoad().

*/


// NOTE: this module is not thread-safe!


// call before starting to register tasks.
// this routine is provided so we can prevent 2 simultaneous load operations,
// which is bogus. that can happen by clicking the load button quickly,
// or issuing via console while already loading.
void BeginRegistering();


// callback function of a task; performs the actual work.
//
// return semantics:
// - if the entire task was successfully completed, return 0;
//   it will then be de-queued.
// - if the work can be split into smaller subtasks, process those until
//   <time_left> is reached or exceeded and then return an estimate
//   of progress in percent (<= 100, otherwise it's a warning;
//   != 0, or it's treated as "finished")
// - on failure, return a negative error code or 'warning' (see above);
//   PS::Loader::ProgressiveLoad will abort immediately and return that.
using LoadFunc = std::function<int()>;

// register a task (later processed in FIFO order).
// <func>: function that will perform the actual work; see LoadFunc.
// <description>: user-visible description of the current task, e.g.
//   "Loading Textures".
// <estimated_duration_ms>: used to calculate progress, and when checking
//   whether there is enough of the time budget left to process this task
//   (reduces timeslice overruns, making the main loop more responsive).
void Register(LoadFunc func, std::wstring description, int estimated_duration_ms);


// call when finished registering tasks; subsequent calls to
// PS::Loader::ProgressiveLoad will then work off the queued entries.
void EndRegistering();


// immediately cancel this load; no further tasks will be processed.
// used to abort loading upon user request or failure.
// note: no special notification will be returned by PS::Loader::ProgressiveLoad.
void Cancel();

struct ProgressiveLoadResult
{
	/**
	 * @c INFO::All_COMPLETE if the final load task just completed.
	 * @c ERR::TIMED_OUT if loading is in progress but didn't finish.
	 * @c 0 if not currently loading (no-op).
	 * Otherwise an error code. the request has been de-queued.
	 */
	Status status{0};

	/**
	 * An empty string when finished.
	 * Otherwise the description of the next task that will be undertaken.
	 */
	std::wstring nextDescription;

	/**
	 * The current progress value.
	 */
	int progressPercent;
};
/**
 * Process as many of the queued tasks as possible within @c timeBudget [s].
 * if a task is lengthy, the budget may be exceeded. call from the main loop.
 */
ProgressiveLoadResult ProgressiveLoad(double time_budget);

// immediately process all queued load requests.
// returns 0 on success or a negative error code.
Status NonprogressiveLoad();


// boilerplate check-if-timed-out and return-progress-percent code.
// completed_jobs and total_jobs are ints and must be updated by caller.
// assumes presence of a local variable (double)<end_time>
// (as returned by timer_Time()) that indicates the time at which to abort.
#define LDR_CHECK_TIMEOUT(completed_jobs, total_jobs)\
	if(timer_Time() > end_time)\
	{\
		size_t progress_percent = ((completed_jobs)*100 / (total_jobs));\
		/* 0 means "finished", so don't return that! */\
		if(progress_percent == 0)\
			progress_percent = 1;\
		ENSURE(0 < progress_percent && progress_percent <= 100);\
		return (int)progress_percent;\
	}

} // namespace PS::Loader

#endif	// #ifndef INCLUDED_LOADER
