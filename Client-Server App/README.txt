
		Aplicatie client-server

	~ server.cpp

- cream cei 2 socketi: unul pentru clientii UDP, celalalt pentru clientii TCP
- dezactivam Nagle
- folosim o multime de concepte pe care le-am invatat la laborator si consider
ca nu trebuie explicate atat de in detaliu (+ sunt multe comentarii sugestive
in cod)
- pentru usurinta manipularii datelor, am ales sa folosesc maps (4 of them
even)

pentru pastrarea clientilor online:

	- online_subs:
		key: subscriber_ID
		value: subscriber_fd
	
	(inversa hartii de mai sus, pentru usurinta gasirii unui id
fara a trece prin toata harta):

	- online_subs_fd:
		key: subscriber_fd
		value: subscriber_ID

pentru pastrarea clientilor offline:
	
	- offline_subs:
		key: subscriber_ID
		value: buffer for keeping offline messages

pentru topicuri:

	- topics:
		key: topic
		value: another map which pairs a subscriber ID with their
			chosen SF parameter for that particular topic 


- incepe while(1):
	- forul practic itereaza prin toate posibilitatile pe care le avem:

~ i == sockfd_TCP => a aparut o noua conexiune => un subscriber s-a logat
sau relogat
	- daca s-a logat pentru prima oara (nu exista in lista offline_subs),
il adaugam in listele cu cei online
	- daca s-a relogat (exista in offline_subs) => ii trimitem mesajele
pe care le-a primit cat timp a fost offline (daca are mesaje de primit),
il scoatem dintre offline_subs, il adaugam la maps cu cei online

~ i == sockfd_UDP => clientii UDP ne trimit mesaje =>
	- luam mesajul, il parsam
		-> nu exista topicul mesajului => drop
		-> exista => iteram prin toti userii care sunt abonati la acel
topic:
			- sunt online => trimitem mesajele
			- nu sunt online => daca SF = 1 pentru clientul
respectiv la topicul respectiv, ii pastram mesajele offline, daca nu -> drop

~ i == 0 => serverul a primit input de la tastatura
	- daca este comanda exit, inchide toate conexiunile si socketii,
se inchide si pe el :)

~ else => suntem pe unul din sock_fd ai subscriberilor => vedem ce comanda dau
serverului:
	- subscribe:
		-> tinem evidenta topicului:
			-> daca topicul nu a fost creat, il cream si adaugam
subscriberul
			-> daca topicul a fost creat, in caz ca subscriberul
deja a dat subscribe la acel topic (poate doar a vrut sa schimbe SF),
stergem vechiul entry. Cream noul entry (id, sf) indiferent care a fost pasul
precedent
	- unsubscribe:
		-> nu exista topicul: go on
		-> exista topicul, sterge clientul dintre abonati (daca exista
printre ei)




	~ subscriber.cpp

- pentru subscriber, mecanismul de select este foarte asemanator, dar mult mai
simplu:

~ i == sockfd => primeste un mesaj pe acea conexiune.
	- am facut in asa fel incat primele maxim 10 caractere (pana la spatiu)
sa fie message size, tinand cont ca recv poate sa intoarca oricate caractere.
Astfel, il vom obliga sa se repete pana citeste tot mesajul. Il printeaza
(fara primii octeti pentru message_size, evident).

~ i == 0 => primeste un mesaj de la tastatura. Acesta poate fi:

	- subscribe topic SF
	- unsubscribe topic
	- exit

	Consider ca am tratat cazurile in care poate sa dea eroare oricare din
comenzile de mai sus in functia "verify_command". Incercate si testate. 
Clientul va primi si sugestii in functie de comenzile eronate pe care le da.


	~ message.cpp/h

- se ocupa cu parsarea mesajelor in dupa cum este specificat in cerinta



P.S. Am testat cu toate inputurile din fisierele .json, si pe random, si cat
sunt clientii offline. Pare sa se descurce la flux mare de date si sa nu
piarda mesaje. :)

Mentionez ca am setat taburile la 2 spatii pentru a ma incadra in limita de
80 de caractere (altfel ar fi trebuit sa sparg aproape toate liniile in doua).
Sper ca nu vor fi decalate liniile. La mine arata ok.
