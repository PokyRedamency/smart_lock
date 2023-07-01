main:main.o SmartLock.o mysql.o open_sockfd.o
	g++ main.o SmartLock.o mysql.o open_sockfd.o -o main -I /usr/include/mysql -L /usr/lib64/mysql -lmysqlclient -lpthread -ldl -lssl -lcrypto -lresolv -lm -lrt
main.o:main.cpp
	g++ -c main.cpp
SmartLock.o:SmartLock.cpp
	g++ -c SmartLock.cpp
mysql.o:mysql.cpp
	g++ -c mysql.cpp
open_sockfd.o:open_sockfd.cpp
	g++ -c open_sockfd.cpp

clean:
	rm -f *.o
	rm -f main
