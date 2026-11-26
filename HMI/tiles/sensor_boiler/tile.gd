extends Tile

# ---- CONSTANTES ----
const OCTET_SIZE = 4
const DEVICE_NAME = "sensor_f"
const SUB_HB = "hb_" + DEVICE_NAME
const SUB_STATUS_COUNTER = "status_" + DEVICE_NAME + "_counter"
const SUB_CMD_RESET = "cmd_" + DEVICE_NAME + "_reset_counter"
const SUB_STATUS_TEMPERATURE = "status_" + DEVICE_NAME + "_temperature"
const SUB_STATUS_HUMIDITY = "status_" + DEVICE_NAME + "_humidity"
const SUB_CMD_LED_POWER = "cmd_" + DEVICE_NAME + "_led_power"
const SUB_STATUS_LED_POWER = "status_" + DEVICE_NAME + "_led_power"
const MAX_LED_POWER = 255
const MASK_Color = Color(0.5, 0.5, 0.5, 0.5)
const HB_TIMEOUT_S := 3.5
const HB_BLINK_S := 0.5

# ---- VARIABLES ----
var _node_id: int = -1
var _seconds_since_hb := INF
var _led_is_on: bool = false
var _connected: bool = false:
	set(value):
		if value == _connected: return
		_connected = value
		if _connected:
			_on_sensor_connected()
		else:
			_on_sensor_disconnected()


# ---- FONCTIONS ----
# Initialise les interactions de l'interface et associe les boutons aux commandes envoyées au capteur.
func _ready() -> void:
	super() 
	%ResetCounterButton.pressed.connect(func(): 
		var buffer = StreamPeerBuffer.new()
		buffer.put_32(int(%ResetCounterLineEdit.text))
		publish(SUB_CMD_RESET, buffer.data_array, true) )
	%RandomLedPowerButton.pressed.connect(func(): 
		var buffer = StreamPeerBuffer.new()
		buffer.put_32(randi_range(0, MAX_LED_POWER))
		buffer.put_32(randi_range(0, MAX_LED_POWER))
		buffer.put_32(randi_range(0, MAX_LED_POWER))
		publish(SUB_CMD_LED_POWER, buffer.data_array, true)
		_led_is_on = true )
	%TurnOffLedPowerButton.pressed.connect(func(): 
		var buffer = StreamPeerBuffer.new()
		buffer.put_32(int(%RedLedPowerSpinBox.value) if !_led_is_on else 0)
		buffer.put_32(int(%GreenLedPowerSpinBox.value) if !_led_is_on else 0)
		buffer.put_32(int(%BlueLedPowerSpinBox.value) if !_led_is_on else 0)
		publish(SUB_CMD_LED_POWER, buffer.data_array, true)
		_led_is_on = !_led_is_on )
		
		
# Déclare les sujets MPS auxquels l'interface doit s'abonner pour recevoir les données du capteur.
func get_subscriptions() -> PackedStringArray:
	var subscriptions: PackedStringArray = []
	
	subscriptions.append(SUB_HB)
	subscriptions.append(SUB_STATUS_COUNTER)
	subscriptions.append(SUB_STATUS_TEMPERATURE)
	subscriptions.append(SUB_STATUS_HUMIDITY)
	subscriptions.append(SUB_STATUS_LED_POWER)
	return subscriptions


# Identifie le capteur lors de sa connexion à la passerelle et actualise son identifiant dans l'interface.
func on_node_online(node_id: int, _mac_address: String, device_name: String) -> void:
	if device_name != DEVICE_NAME: return
	_node_id = node_id  
	%IDLabel.text = "node_id: %d" % _node_id


# Détecte la déconnexion du capteur et actualise son état de connexion.
func on_node_offline(node_id: int, _reason_code: int) -> void:
	if node_id == _node_id: _connected = false


# Gère la perte de communication avec la passerelle et réinitialise l'identifiant du capteur.
func on_gateway_reachability_changed(is_gateway_reachable: bool) -> void:
	if is_gateway_reachable: return
	_connected = false
	_node_id = -1 


# Traite les messages reçus du capteur, vérifie leur origine et met à jour les valeurs affichées dans l'interface.
func on_message(subject_name: String, source_id: int, payload: PackedByteArray) -> void:
	if subject_name == SUB_HB: _on_hb(source_id)
	if source_id != _node_id: return 
	if subject_name == SUB_STATUS_COUNTER:
		if payload.size() != OCTET_SIZE: return 
		%CounterLabel.text = "Compteur : %d" % payload.decode_u32(0)
	if subject_name == SUB_STATUS_TEMPERATURE:
		if payload.size() != OCTET_SIZE: return
		%TemperatureLabel.text = "Température : %.1f °C" % payload.decode_float(0)
	if subject_name == SUB_STATUS_HUMIDITY:
		if payload.size() != OCTET_SIZE: return
		%HumidityLabel.text = "Humidité : %.1f %%" % payload.decode_float(0)
	if subject_name == SUB_STATUS_LED_POWER:
		if payload.size() != OCTET_SIZE * 3: return
		var red = payload.decode_u32(0)
		var green = payload.decode_u32(OCTET_SIZE)
		var blue = payload.decode_u32(OCTET_SIZE * 2)
		%RedLedPowerSpinBox.value = red
		%GreenLedPowerSpinBox.value = green
		%BlueLedPowerSpinBox.value = blue

	
# Surveille la réception des heartbeats afin de détecter une éventuelle perte de communication avec le capteur.
func _process(delta: float) -> void:
	_seconds_since_hb += delta
	%HeartbeatIcon.state = _seconds_since_hb < HB_BLINK_S
	if _connected and _seconds_since_hb >= HB_TIMEOUT_S:
		_connected = false  

# Active ou désactive récursivement les éléments interactifs d'un conteneur de l'interface.
func set_controls_enabled(node: Node, enabled: bool) -> void:
	for child in node.get_children():
		if child is LineEdit:
			child.editable = enabled
		elif child is TextEdit:
			child.editable = enabled
		elif child is BaseButton:
			child.disabled = not enabled
		elif child is Range:
			child.editable = enabled
		set_controls_enabled(child, enabled)

# Met à jour l'interface lors de la connexion du capteur et autorise l'utilisation de ses commandes.
func _on_sensor_connected():
	%IDLabel.text = "node_id: %d" % _node_id
	%ConnectedIcon.state = true
	
	%HBoxContainer.modulate = Color(1, 1, 1, 1)
	set_controls_enabled(%HBoxContainer, true)


# Met l'interface en état déconnecté et désactive les commandes tant que le capteur n'est pas disponible.
func _on_sensor_disconnected():
	%IDLabel.text = "node_id: ∅"
	%ConnectedIcon.state = false
	
	%HBoxContainer.modulate = MASK_Color
	set_controls_enabled(%HBoxContainer, false)

# Actualise le délai de réception du heartbeat et confirme la connexion lorsque le message provient du capteur attendu.
func _on_hb(source_id: int):
	
	if _node_id == -1: _node_id = source_id
	if source_id != _node_id: return
	_seconds_since_hb = 0.0
	_connected = true

	
