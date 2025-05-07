class AutoStartHost
{
	done = false;
	constructor(cmdLineArgs)
	{
		this.launched = false;
		this.playerAssignments = {};

		this.maxPlayers = +(cmdLineArgs['autostart-host-players'] ?? 2);
		this.cmdLineArgs = cmdLineArgs;

		try
		{
			const playerName = cmdLineArgs['autostart-playername'] || "anonymous";
			const port = +(cmdLineArgs['autostart-port'] ?? 5073);

			// Password not implemented for autostart.
			Engine.StartNetworkHost(playerName, port, "", false, !('autostart-disable-replay' in cmdLineArgs));
		}
		catch(e)
		{
			const message = sprintf(translate("Cannot host game: %(message)s."), { "message": e.message });
			messageBox(400, 200, message, translate("Error"));
		}

		/**
		 * Handles a simple implementation of player assignments.
		 * Should not need be overloaded in mods unless you want to change that logic.
		 */
		(async() =>
		{
			while (true)
			{
				const message = await Engine.PollNetworkClient();
				switch (message.type)
				{
				case "players":
				{
					this.playerAssignments = message.newAssignments;
					Engine.SendNetworkReady(2);
					let max = 0;
					for (const uid in this.playerAssignments)
					{
						max = Math.max(this.playerAssignments[uid].player, max);
						if (this.playerAssignments[uid].player == -1)
							Engine.AssignNetworkPlayer(++max, uid);
					}
					break;
				}
				case "ready":
					this.playerAssignments[message.guid].status = message.status;
					break;
				case "start":
					this.done = true;
					return;
				default:
				}

				if (!this.launched)
				{
					const assignementArray = Object.values(this.playerAssignments);
					if (assignementArray.length === this.maxPlayers &&
						assignementArray.every(assignement =>
							assignement.player !== -1 || assignement.status !== 0))
					{
						this.onLaunch();
					}
				}
			}
		})();
	}

	onTick()
	{
		return this.done;
	}

	/**
	 * In the visual autostart path, we need to show the loading screen.
	 * Overload this as appropriate.
	 */
	onLaunch()
	{
		this.launched = true;

		this.settings = new GameSettings().init();

		parseCmdLineArgs(this.settings, this.cmdLineArgs);

		this.settings.playerCount.setNb(Object.keys(this.playerAssignments).length);
		this.settings.launchGame(this.playerAssignments, this.storeReplay);
		Engine.SwitchGuiPage("page_loading.xml", {
			"attribs": this.settings.finalizedAttributes,
			"playerAssignments": this.playerAssignments
		});
	}
}
