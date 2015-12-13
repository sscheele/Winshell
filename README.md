# Winshell
Netcat-like shell (with file transfer support) designed to run on Windows

Winshell is an educational program that I wrote when AV kept picking up Netcat. In addition to Netcat's AV problems, it's not exclusively a backdoor, so it doesn't support uploading files to (or stealing them from) a victim. Winshell isn't done (at the moment, it's very nearly unusable), and I don't condone its illegal use.

Winshell's server component takes no arguments at the moment, although it should eventually take one. For now, just run it from the command line.

Winshell's client component takes three arguments: the server IP, the server port, and the time to wait (in milliseconds) before trying to reconnect.

Winshell doesn't support encryption at the moment, and I don't have any plans to add it. However, an argument could be made that although Netcat is much better written, Winshell is more secure as a backdoor because the victims are clients, not servers. They'll connect to your server's IP, which means that other people would have to try (a little) harder to access your victims.
