function CameraShake() {}

CameraShake.prototype.Schema =
	"<element name='Duration'>" +
		"<ref name='nonNegativeDecimal'/>" +
	"</element>";

CameraShake.prototype.Init = function()
{
	this.owner = INVALID_PLAYER;
};

CameraShake.prototype.OnOwnershipChanged = function(msg)
{
	// Ownership is cleared before the Destroy message reaches every component.
	// Keep the last real owner so OnDestroy can still address the notification.
	if (msg.to != INVALID_PLAYER)
		this.owner = msg.to;
};

CameraShake.prototype.OnDestroy = function()
{
	let cmpHealth = Engine.QueryInterface(this.entity, IID_Health);

	// Ignore editor removal and player deletion.
	if (!cmpHealth || cmpHealth.GetHitpoints() > 0)
		return;

	if (this.owner == INVALID_PLAYER)
		return;

	Engine.QueryInterface(SYSTEM_ENTITY, IID_GuiInterface).PushNotification({
		"type": "camera-shake",
		"players": [this.owner],
		"duration": +this.template.Duration
	});
};

Engine.RegisterComponentType(IID_CameraShake, "CameraShake", CameraShake);
