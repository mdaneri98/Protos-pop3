# POP3 Server v1.0 ITBA - 72.07 Protocolos de Comunicación 20232Q

---

| Alumno | Legajo | Mail |
| --- | --- | --- |
| Arnaude, Juan Segundo | 62184 | jarnaude@itba.edu.ar |
| Canevaro, Bautista | 62179 | bcanevaro@itba.edu.ar |
| Daneri, Matías Ezequiel | 60798 | mdaneri@itba.edu.ar |
| Wodtke, Matías | 62098 | mwodtke@itba.edu.ar |

---
## Compilación y creación de los ejecutables

Para comenzar con la compilación, este proyecto cuenta con un Makefile que facilita este proceso. Para ello, entrar al directorio raíz y ejecutar los siguientes comando. El primero para limpiar los archivos residuales y el segundo para generar los ejecutables necesarios.

```bash
$$ make clean
$$ make all
```

## Uso del directorio de mails

Para poder utilizar correctamente el servidor POP3, se requiere contar con un directorio llamado `Maildir` que contenga todos los correos de los usuarios del sistema. A su vez, dentro de cada una de las carpetas de los usuarios, se debe agregar una carpeta llamada `cur` que incluirá los correos, cada uno en archivos separados.

A continuación, se adjunta un esquema que ilustra los descripto previamente.

```bash
Maildir
└── someuser
    ├── cur
    │   ├── 1700513155.V802I61309M300068.debian_2,S
    │   └── 1700513186.V802I61e75M344960.debian_2,
    ├── new
    └── tmp
```

A continuacion se presenta un ejemplo de uso:

```bash
$$ ./pop3d -u mdaneri:pass123 -u someone:pass321 -d mails -v -t 123456
```

Para conectarnos al servidor pop3d y utilizarlo.

```bash
$$ nc -C localhost [SERVER_PORT]

```

La flag `-C` es esencial para el funcionamiento ya que envía el ASCII '\r' antes de enviar '\n'.

---

## Ejecución de la aplicación servidor

Antes que nada se debe iniciar el programa servidor, `pop3d`.

```bash
$$ ./pop3d [ARGUMENTS] -t [TOKEN]
```
Los argumentos aceptados son los siguientes:

```bash
--help
-h                               This help message.
--directory <maildir>
-d <maildir>                     Path to directory where it'll find all users with their mails. (Optional)
--pop3-server-port <pop3 server port>
-p <pop3 server port>            Port for POP3 server connections.(Optional)
--config-server-port <configuration server port>
-P <configuration server port>   Port for configuration client connections.(Optional)
--user <user>:<password>
-u <user>:<password>             User and password for a user which can use the POP3 server. Up to 10.(Optional)
--token <token>
-t <token>                       Authentication token for the client. 
--version
-v                               Prints version information.(Optional)
```

`-t` o `--token` es el único argumento requerido. El puerto default del servidor es: `1110`.

Ejemplo de uso del servidor.

```bash
$$ ./pop3d -u mdaneri:pass123 -u someone:pass321 -d mails -v -t 123456
```

---

## Ejecución del cliente

Una vez el programa servidor esté en ejecución, se corre el siguiente comando.
```bash
$$ ./client -t [TOKEN] [ARGUMENTS]
```
Los argumentos aceptados son los siguientes.
```bash
token [token_value] help|not-given:not-given
token [token_value] add-user|[username]:[password]
token [token_value] change-pass|[username]:[new_password]
token [token_value] change-maildir|[new_maildir]:not-given
token [token_value] remove-user|[username]:not-given
token [token_value] not-given|not-given:not-given
token [token_value] version|not-given:not-given
token [token_value] get-max-mails|not-given:not-given
token [token_value] set-max-mails|[new_max_mails]:not-given
token [token_value] stat-historic-connections|not-given:not-given
token [token_value] stat-bytes-transferred|not-given:not-given
```

`-t` o `--token` es el único argumento requerido.

---

## Conexión al servidor pop3

```bash
$$ nc -C localhost [SERVER_PORT]
```

La flag `-C` es esencial para el funcionamiento ya que envia es ASCII '\r' antes de enviar '\n'.