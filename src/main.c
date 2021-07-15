#include "daemonization.h"
#include "snmp_printers.h"

int main(int argc, char** argv) {
    printf("%d\n", argc);
    char *cmd;

    if ((cmd = strrchr(argv[0], '/')) == NULL) {
        cmd = argv[0];
    } else {
        cmd++;
    }

    daemonize(cmd);

    if (already_running()) {
        syslog(LOG_ERR, "printers supervisor daemon is already running");
        exit(EXIT_FAILURE);
    }

    printer_t printers_ips[MAX_PRINTERS_AMOUNT];
    int printers_amount = 0;
    if (scan_network_printers(&printers_ips, &printers_amount)) {
        syslog(LOG_ERR, "too much printer IPs(%d is max) or there is no network printers available!", MAX_PRINTERS_AMOUNT);
        exit(EXIT_FAILURE);
    }

    while (TRUE) {
        sleep(TIMEOUT);
        check_printers_state(printers_ips, printers_amount);
        syslog(LOG_INFO, "printers supervisor daemon is running!");
    }

    return EXIT_SUCCESS;
}
