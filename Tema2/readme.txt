Ghilusi Corina-Adriana
323CDa

subscriber.cpp
In subscriber vom lucra cu doua structuri: send_msg si udp_msg. Scopul structurii send_msg
este facilitarea trimiterii unui mesaj de subscribe sau unsubscribe catre server. Structura
udp_msg ajuta la receptionarea mesajelor provenite de la clientul udp. Serverul primeste
mesajul ca string si il trimite subscriber-ului sub aceasta forma, urmand ca singura
modificare ce trebuie facuta asupra acestuia este convertirea tipului in echivalentul
sau, fapt realizat de functia convert.

server.cpp
In server, vom relua cele doua structuri mentionate anterior, insa de data aceasta send_msg
are scopul de a receptiona.
Server-ul poate citi de la tastatura doar comanda exit, caz in care se inchid atat acesta,
cat si clientii ce sunt inca conectati.
    Server-ul accepta conexiunea clientului pe socket-ul tcp.Avem o lista de structuri tip client in care vor
fi retinuti toti clientii si pe care o vom modifica in functie de comportamentul acestora. Verificam daca este vorba despre un
client nou, caz in care il adaugam in lista de clienti, sau despre un client ce se reconecteaza.
In cazul clientului ce se reconecteaza, il setam pe acesta drept conectat (pe baza campului isConnected) si ii modificam
socket-ul (campul personalSocket) cu valoarea celui curent. In cazul in care un client doreste sa se conecteze cu un ID
cu care se afla conectat un alt client, il vom deconecta.
Pentru a retine topic-urile, vom utiliza o lista de structuri de tip topic. Fiecare element
de tip topic retine o lista de subscriberi de tip client. Atunci cand un client se reconecteaza,
vom avea grija sa parcurgem si subscriberii topicurilor, sa cautam daca clientul reconectat se afla
printre acestia, caz in care, il reconectam si acolo.
Vom utiliza, de asemenea, si o lista de structuri de tip storedMsg, in care vom retine mesaje udp ce
apar atunci cand un client conectat cu SF=1 se deconecteaza. In cazul reconectarii, vom trimite
clientului aceste mesaje ce au aparut cat timp era deconectat.
    Putem primi de la subscriber 3 tipuri de mesaje: exit, caz in care utilizatorul se deconecteaza,
subscribe, caz in care este adaugat in lista de subscriberi a topicului dorit; si unsubscribe, caz
in care se dezaboneaza si este eliminat din lista de subscriberi a topiculi respectiv.
    In cazul unui mesaj udp, vom utiliza structura udp_msg, menita sa retina compartimentat toate
datele ce vor fi trimise subscriber-ului. Realizam conversia in functie de tipul de date primit ca
numar. Verificam daca topicul ce genereaza update-uri exista, daca nu, il vom crea. Daca acesta exista,
vom parcurge lista de subscriberi ai acestuia si le vom trimite update-ul, daca sunt conectati. Daca
vreunul dintre subscriberi este deconectat, il cautam dupa id si topic in lista de storedMsg. Daca il
gasim, inseamna ca acesta doreste sa ii fie pastrate update-urile ce apar in absenta sa, asa ca
stocam in lista de mesaje udp (history) acest mesaj, pe care il va primi la reconectare.

    Feedback:
    Checker-ul are de cele mai multe ori un comportament variabil, putand oferi rezultate diferite
pe baza aceluiasi cod care nu prezinta probleme la o testare manuala. In momentul trimiterii, singurele
teste care pica sunt quick_flow si c2_restart_sf :).
