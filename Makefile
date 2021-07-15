CC=gcc
WORKDIR=src
INCDIR=-I./include
FLAGS=-Wall -Werror -pthread
OUTPUT=printers_supervisor.exe
DAEMON_PID=$(shell cat /var/run/printers_supervisor_daemon.pid)

app.exe:
	$(CC) $(FLAGS) $(INCDIR) $(WORKDIR)/snmp_printers.c $(WORKDIR)/daemonization.c $(WORKDIR)/main.c -o $(OUTPUT)

run: $(OUTPUT)
	sudo ./$(OUTPUT)
}

kill:
	sudo kill -9 $(DAEMON_PID)

log:
	sudo cat /var/log/syslog

clean:
	rm *.out *.exe *.o

rerun: $(OUTPUT)
	sudo rm -rf $(OUTPUT)
	sudo make
	sudo kill -9 $(DAEMON_PID)
	sudo ./app.exe
