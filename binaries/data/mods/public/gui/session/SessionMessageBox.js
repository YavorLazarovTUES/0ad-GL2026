/**
 * This is the same as a regular MessageBox, but it pauses if it is
 * a single-player game, until the player answered the dialog.
 */
class SessionMessageBox
{
	async display(closePageCallback)
	{
		closeOpenDialogs();
		g_PauseControl.implicitPause();

		const buttonId = await Engine.OpenChildPage(
			"page_msgbox.xml",
			{
				"width": this.Width,
				"height": this.Height,
				"title": this.Title,
				"message": this.Caption,
				"buttonCaptions": this.Buttons ? this.Buttons.map(button => button.caption) : undefined,
			});

		if (this.Buttons && this.Buttons[buttonId].onPress)
		{
			const ret = this.Buttons[buttonId].onPress.call(this);
			if (ret !== undefined)
				closePageCallback({ [Engine.openRequest]: ret });
		}

		if (this.ResumeOnClose)
			resumeGame();
	}
}

SessionMessageBox.prototype.Width = 400;
SessionMessageBox.prototype.Height = 200;

SessionMessageBox.prototype.ResumeOnClose = true;
