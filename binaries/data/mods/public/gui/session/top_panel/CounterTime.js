class CounterTime
{
	constructor(resCode, panel, icon, count, stats)
	{
		this.resCode = resCode;
		this.panel = panel;
		this.icon = icon;
		this.count = count;
		this.stats = stats;

		
		this.stats.caption = "";
	}

	rebuild(playerState, getAllyStatTooltip)
	{
		this.count.caption = Engine.FormatMillisecondsIntoDateStringGMT(
			g_SimState.timeElapsed, translate(this.TimeFormat));

		this.panel.tooltip =
			setStringTags(translate(this.TimeTitle), CounterManager.ResourceTitleTags) +
			"\n" + translate(this.TimeTooltip);
	}
}


CounterTime.prototype.TimeFormat = markForTranslation("H:mm:ss");

CounterTime.prototype.TimeTitle = markForTranslation("Elapsed Time");

CounterTime.prototype.TimeTooltip = markForTranslation("Time elapsed since the game started. It stops while the game is paused.");
