Client-server application

~ server.cpp

- create the 2 sockets: one for UDP clients, the other for TCP clients
- disable Nagle
- for ease of data manipulation, I chose to use maps (4 of them even)

for keeping customers online:

- online_subs:
key: subscriber_ID
value: subscriber_fd

(reverse the map above, for ease of finding an id
without going through the whole map):

- online_subs_fd:
key: subscriber_fd
value: subscriber_ID

to keep customers offline:

- offline_subs:
key: subscriber_ID
value: buffer for keeping offline messages

for topics:

- topics:
key: topic
value: another map which pairs a subscriber ID with their
chosen SF parameter for that particular topic


- start while (1):
- the practical forum iterates through all the possibilities we have:

~ i == sockfd_TCP => a new connection has appeared => a subscriber has signed up
or logged in
- sign up (does not exist in the offline_subs list): add to online lists
- login (exists in offline_subs) => send the messages received while offline 
(if there are messages to be received), remove from offline_subs, 
add to the maps with the online subs

~ i == sockfd_UDP => UDP clients send us messages =>
- we take the message, we share it
-> the message topic doesn't exist => drop
-> the message topic exists => iterates through all users who subscribed to that
topic:
- they are online => we send the messages
- they are not online => if SF = 1 for the client to the respective topic,
we keep their messages offline, if not -> drop

~ i == 0 => the server received keyboard input
- if exit command, close all connections and sockets

~ else => sock_fd of the subscribers => we see what order they give to the server:
- subscribe:
-> we keep track of the topic:
-> if the topic was not created, we create it and add it
Subscriber
-> if the topic was created, in case the subscriber
already subscribed to that topic (maybe they just wanted to change SF),
we delete the old entry. Creayr the new entry (id, sf)
- unsubscribe:
-> there is no topic: go on
-> there is a topic, delete the client from subscribers (if they are
among them)




~ subscriber.cpp

- for the subscriber, the selection mechanism is very similar, but simpler:

~ i == sockfd => receives a message on that connection.
- I made it so that the first maximum 10 characters (up to space)
will be the size of the message, considering that recv can return any number
of characters.

~ i == 0 => receives a message from the keyboard. This can be:

- subscribe topic SF
- unsubscribe topic
- exit

~ message.cpp / h

- deals with parsing messages

