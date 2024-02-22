# SimulationNs3
Simulation d'un réseau domestique.
Ce scénario simule le fonctionnement d'un réseau domestique constitué de deux ordinateurs générant un trafic HTTP et une télévision IP générant un trafic RTSP connecté aux serveurs à travers un point acces wifi.

Le travail a été réalisé sur NS-3.35;

Pour réaliser la simulation, il faut avoir les fichiers frame.txt et ReseauDomestique.cc  dans votre dossier scratch et avoir le module RTSP installé dans NS-3.

Ensuite, vous pouvez lancer la simulation en executant: 
"./waf --run scratch/ReseauDomestique"
