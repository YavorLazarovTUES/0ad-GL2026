class CounterTime
{
	constructor(resCode, panel, icon, count, stats)
	{
		this.resCode = resCode;
		this.panel = panel;
		this.icon = icon;
		this.count = count;
		this.stats = stats;

		// The clock has its own compact layout instead of using the larger
		// resource-icon dimensions, which collide with the elapsed-time text.
		let iconSize = this.icon.size;
		iconSize.left = 4;
		iconSize.top = 3;
		iconSize.right = 32;
		iconSize.bottom = 31;
		this.icon.size = iconSize;

		let countSize = this.count.size;
		countSize.left = 36;
		countSize.right = 105;
		this.count.size = countSize;

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
