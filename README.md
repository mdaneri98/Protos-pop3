POP3 Server v1.0
ITBA - 72.07 Protocolos de Comunicación 20232Q

Juan Segundo Arnaude        62184       jarnaude@itba.edu.ar
Matias Wodtke               62098       mwodtke@itba.edu.ar
Matias Ezequiel Daneri      60798       mdaneri@itba.edu.ar
Bautista Canevaro           62179       bcanevaro@itba.edu.ar

Compilación y creación de los ejecutables

Para comenzar con la compilación, este proyecto cuenta con un Makefile que facilita este proceso. Para ello, entrar al directorio raíz y ejecutar los siguientes comando. El primero para limpiar los archivos residuales y el segundo para generar los ejecutables necesarios.

$$ make clean
$$ make all


Uso del directorio de mails

Para poder utilizar correctamente el servidor POP3, se requiere contar con un directorio llamado Maildir que contenga todos los correos de los usuarios del sistema. A su vez, dentro de cada una de las carpetas de los usuarios, se debe agregar una carpeta llamada cur que incluirá los correos, cada uno en archivos separados.

Maildir
└── someuser
    ├── cur
    │   ├── 1700513155.V802I61309M300068.debian_2,S
    │   └── 1700513186.V802I61e75M344960.debian_2,
    ├── new
    └── tmp


Ejecución de las aplicacion servidor 

Se debe iniciar el programa servidor, `pop3d`. 

$$ ./pop3d [ARGUMENTS] -t [TOKEN]

Los argumentos aceptados son los siguientes:
        
--help
-h                               This help message.
--directory <maildir>
-d <maildir>                     Path to directory where it'll find all users with their mails. 
--pop3-server-port <pop3 server port>
-p <pop3 server port>            Port for POP3 server connections.
--config-server-port <configuration server port>
-P <configuration server port>   Port for configuration client connections.
--user <user>:<password>
-u <user>:<password>             User and password for a user which can use the POP3 server. Up to 10.
--token <token>
-t <token>                       Authentication token for the client. 
--version
-v                               Prints version information.

-t o --token es el unico argumento requerido.
El puerto default del servidor es: 1110.

Ejemplo de uso:

$$ ./pop3d -u mdaneri:pass123 -u someone:pass321 -d mails -v -t 123456

Ejecución del cliente

Una vez el prorama servidor este en ejecuccion se corre el siguiente programa, se corre el siguiente comando:

$$ ./client -t [TOKEN] [ARGUMENTS]

Los argumentos aceptados son los siguientes:

--help
-h                               This help message.
--config-server-port <configuration server port>
-P <configuration server port>   Port for configuration client connections
--token <token>
-t <token>                       Authentication token for the client.
--version
-v                               Prints version information.
-add-user <user>:<password>
-u <user>:<password>             Add a user to the server.
--change-pass <user>:<password>
-p <user>:<password>             Change the password of a user.
--remove-user <user>
-r <user>                        Remove a user from the server.
--change-maildir <maildir>
-m <maildir>                     Change mail directory.
--get-max-mails 
-g                              Get the maximum number of mails.
--set-max-mails <number>
-s <number>               Set the maximum number of mails.
--stat-historic-connections
-i                       Get the number of historic connections.
--stat-current-connections
-c                       Get the number of current connections.
--stat-bytes-transferred
-b                       Get the number of bytes transferred.

-t o --token es el unico argumento requerido.

Conexion al servidor pop3d

$$ nc -C localhost [SERVER_PORT]

La flag -C es esencial para el funcionamiento ya que envia es ASCII '\r' antes de enviar '\n'.

