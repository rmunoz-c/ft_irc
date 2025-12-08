//TODO COMMANDS!
//-C- A TRABAJAR! -____________-


✔ PASS

Envía la contraseña para poder conectarse al servidor.

✔ NICK

Establece o cambia el nickname del usuario.

✔ USER

Envía la información de registro del usuario.
Necesario para completar la autenticación.

✔ PING

El cliente comprueba que el servidor está vivo.
Tu servidor debe responder con PONG.

✔ PONG

Respuesta del cliente cuando el servidor envía un PING.

✔ JOIN

El usuario entra en un canal.
Ejemplo:
JOIN #general

✔ PART

El usuario SALE de un canal.

✔ PRIVMSG

Enviar un mensaje privado a:

un usuario

un canal

Ej: PRIVMSG #general :Hola a todos

✔ NOTICE

Como PRIVMSG pero sin respuestas automáticas.

✔ QUIT

El usuario cierra la sesión en el servidor.

✅ 2. Comandos que deben soportar los OPERADORES DE CANAL (OP)

(Los operadores son usuarios con privilegios dentro de un canal, no del servidor)

Estos son exactamente los que exige el subject:

✔ KICK

Expulsa a un usuario del canal.
KICK #canal usuario :razón

✔ INVITE

Invita a un usuario a entrar al canal.
INVITE usuario #canal

✔ TOPIC

Cambia o muestra el tema del canal.
TOPIC #canal :Nuevo tema

✔ MODE

Gestiona los modos del canal.
El subject exige implementar:

🔧 Modos obligatorios de canal (todos pertenecen a MODE)
✔ i — Invite-only

El canal solo acepta usuarios invitados.

MODE #canal +i
MODE #canal -i

✔ t — Solo OP puede cambiar TOPIC
MODE #canal +t
MODE #canal -t

✔ k — Establecer/eliminar clave del canal (password)
MODE #canal +k contraseña
MODE #canal -k

✔ o — Dar o quitar OP a un usuario
MODE #canal +o usuario
MODE #canal -o usuario

✔ l — Establecer o eliminar límite de usuarios
MODE #canal +l 10
MODE #canal -l