# Projet_Godot

### `kSubHb`

Ce subject permet au capteur de signaler régulièrement sa présence à la passerelle et à l'interface de supervision. Il s'agit d'un message de type heartbeat, envoyé toutes les secondes. Il ne contient aucun payload de données métier et permet de vérifier que le périphérique est toujours actif.

### `kSubStatusCounter`

Ce subject permet au capteur de transmettre la valeur actuelle de son compteur à l'interface de supervision. Le message est envoyé toutes les deux secondes et après chaque modification du compteur. Son payload est composé d'un champ `count` de type `uint32_t`, représentant le nombre d'incréments. Sa taille totale est de 4 octets.

### `kSubCmdResetCounter`

Ce subject permet à l'interface de supervision de demander la modification de la valeur du compteur du capteur. Son payload contient un champ `newValue` de type `uint32_t`, correspondant à la nouvelle valeur du compteur en nombre d'incréments. La taille totale du payload est de 4 octets. Après réception et validation de la commande, le capteur met à jour son compteur et publie sa nouvelle valeur sur le subject `status_sensor_f_counter`.

### `kSubStatusTemperature`

Ce subject permet au capteur de transmettre la température mesurée par le capteur DHT11. Le payload contient un champ `degree` de type `float`, exprimant la température en degrés Celsius (°C). Sa taille totale est de 4 octets. Les données sont publiées toutes les secondes lorsque la lecture du capteur est valide.

### `kSubStatusHumidity`

Ce subject permet au capteur de transmettre le taux d'humidité mesuré par le capteur DHT11. Le payload contient un champ `percentage` de type `float`, exprimant l'humidité relative en pourcentage (%). Sa taille totale est de 4 octets. Les données sont publiées toutes les secondes lorsque la lecture du capteur est valide.

### `kSubCmdLedPower`

Ce subject permet à l'interface de supervision de commander la couleur de la LED RGB du capteur. Son payload contient trois champs de type `uint32_t` : `red`, `green` et `blue`, correspondant respectivement aux intensités des composantes rouge, verte et bleue. Chaque composante possède une valeur comprise entre 0 et 255. La taille totale du payload est de 12 octets. Le capteur vérifie les valeurs reçues avant d'appliquer la couleur demandée.

### `kSubStatusLedPower`

Ce subject permet au capteur de transmettre à l'interface de supervision l'état de sa LED RGB. Son payload reprend les trois champs `red`, `green` et `blue` de type `uint32_t`, représentant les intensités respectives des composantes rouge, verte et bleue, comprises entre 0 et 255. La taille totale du payload est de 12 octets. L'état est publié à l'ouverture d'une session MPS et après chaque modification de la couleur de la LED.