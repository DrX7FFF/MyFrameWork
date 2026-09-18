# MyFrameWorfk
Framework


Besoin principale du Framework :
- pour faire le provisionning wifi
- pas de SSID et de MdP dans le code
- compatible ESP8266 et EPS32
- simple : 1 ligne à ajouter dans le startup
- léger
- option pour que le maintient du bouton "boot" force le mode provisionning

Comportement au boot :
Si pas de crédential enregistré ou bouton "boot" maintenu pendant 5s
	passage en mode provisionning
sinon
	Connection au Wifi

Cas à couvrir : si au bout d'un certain temps le WiFi n'est pas connecté, alors il se remet en mode "provisionning"  (cas où l'utilisateur change de box ou SSID et qu'on ne veut pas reflasher l'ESP) 

De plus, j'aimerais intégrer dans mon FrameWork une option par défaut comme une console SSH, un port sur lequel je me connecte, qui fait défiler les logs et avoir un interpréteur de commande :
- restart
- statut
- DHCP : pour définir si mode DHCP ou IPStatic
- ipadress : pour définir l'adresse IP
- ipmask : pour définir le masque de réseau
- SSID : définir nom + mdp du réseau wifi
...

Et donc je me dis que cette console pourrait me servir pour le provisionning, que l'ESP aurait juste à activer temporairement le AP si pas connecté au WiFi