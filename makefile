exe: Client.c fds/FileDataServer.c fns/FileNameServer.c
	gcc -g Client.c -o bigfs
	gcc -g fds/FileDataServer.c -o fds
	gcc -g fns/FileNameServer.c -o fns

