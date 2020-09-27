# pull-push
a C program for file transfering

[server]
./pull [port] [accept directory]

example:
./pull 8888 /mobile/task/

[client]
./push [host] [port] [transfer file]

example:
./push 192.168.1.107 8888 ./1.zip