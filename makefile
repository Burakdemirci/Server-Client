
All:
	
	gcc -o server server.c uici.c uiciname.c -lnsl -lm -pthread -w
	gcc -o clients client.c uici.c uiciname.c -lnsl -lm -pthread -w

clean:
	rm *.o server clients
