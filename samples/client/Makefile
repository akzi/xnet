CFLAGS = -g -W \
-O3 \
-Wall \
-Wpointer-arith \
-Wshadow \
-Wno-long-long \
-Wformat \
-DACL_PREPARE_COMPILE \
-I ../../include \
-std=c++11

client:rm_client
	g++ -o client client.cpp $(CFLAGS) 
rm_client:
	$(shell if [ -f client ]; then rm client; fi;)

