#include "snmp_printers.h"

void create_request_string(char request_string[REQUEST_STRING_LENGTH], char *ip, char *oid) {
    sprintf(request_string, "%s %s %s", REQUEST_START, ip, oid);
}


FILE* make_request(char *ip, char *oid) {
    char request_string[REQUEST_STRING_LENGTH];
    create_request_string(request_string, ip, oid);
    return popen(request_string, "r");
}

int get_printer_name(printer_t *printer) {
    FILE *fstream = make_request(printer->ip, PRINTER_NAME_OID);
    char buff[BUFFER_LENGTH];    
    memset(buff, 0, sizeof(buff));   

    if (fstream == NULL) {
        return EXIT_FAILURE;
    }

    fgets(buff, sizeof(buff), fstream);
    // Really often result string has an extra part.
    // To get rid of this is used "strstr" and "strlen" functions.
    strcpy(printer->name, strstr(buff, PRINTER_STRING_PREFIX) + strlen(PRINTER_STRING_PREFIX));

    pclose(fstream)

    return EXIT_SUCCESS;
}

int get_printer_resources_names(printer_t *printer) {
    FILE *fstream = make_request(printer->ip, PRINTER_RESOURCES_NAMES);
    char buff[BUFFER_LENGTH];    
    memset(buff, 0, sizeof(buff));   

    if(fstream == NULL) {
        return EXIT_FAILURE;
    }

    printer->resources_amount = 0;
    while (fgets(buff, sizeof(buff), fstream) != NULL) {
        if (printer->resources_amount >= MAX_RESOURCES_AMOUNT) {
            break;
        }
        strcpy((printer->resources)[printer->resources_amount].name,
               strlen(PRINTER_STRING_PREFIX) + strstr(buff, PRINTER_STRING_PREFIX));
        ++(printer->resources_amount);
    }

    pclose(fstream)

    return EXIT_SUCCESS;
}

int get_resources_max_level(printer_t *printer) {
    FILE *fstream = make_request(printer->ip, PRINTER_RESOURCES_MAX_LEVEL);
    char buff[BUFFER_LENGTH];
    memset(buff, 0, sizeof(buff));

    if (fstream == NULL) {
        return EXIT_FAILURE;
    }

    int resource_pos = 0;
    while (fgets(buff, sizeof(buff), fstream) != NULL) {
        (printer->resources)[resource_pos].max = atoi(strstr(buff, PRINTER_INTEGER_PREFIX) +
                                                      strlen(PRINTER_INTEGER_PREFIX));
        resource_pos++;
    }

    pclose(fstream)

    return EXIT_SUCCESS;
}

int get_resources_current_level(printer_t *printer) {
    FILE *fstream = make_request(printer->ip, PRINTER_RESOURCES_CURR_LEVEL);
    char buff[BUFFER_LENGTH];
    memset(buff, 0, sizeof(buff));

    if (fstream == NULL) {
        return EXIT_FAILURE;
    }

    int resource_pos = 0;
    while (fgets(buff, sizeof(buff), fstream) != NULL) {
        (printer->resources)[resource_pos].current = atoi(strstr(buff, PRINTER_INTEGER_PREFIX) +
                                                          strlen(PRINTER_INTEGER_PREFIX));
        resource_pos++;
    }

    pclose(fstream)

    return EXIT_SUCCESS;
}

int add_printer_if_not_duplicate(printer_t (*printers_ips)[MAX_PRINTERS_AMOUNT],
                            int *printers_amount,
                            char ip[PRINTER_IP_LENGTH]) {
    // For some printer models happens, that we have more then one ip
    // for one printer (because of "avahi-browse" ouput) (but they same).
    // Check for duplicates. If we already know about the printer, then just exit the function (no error).
    for (int i = 0; i < *printers_amount; ++i) {
        if (!strcmp(ip, (*printers_ips)[i].ip)) {
            return EXIT_SUCCESS;
        }
    }

    if (*printers_amount == MAX_PRINTERS_AMOUNT) {
        return EXIT_FAILURE;
    }
    strcpy ((*printers_ips)[*printers_amount].ip, ip);
    if (get_printer_name(&(*printers_ips)[*printers_amount]) ||
        get_printer_resources_names(&(*printers_ips)[*printers_amount]) ||
        get_resources_max_level(&(*printers_ips)[*printers_amount])) {
        return EXIT_FAILURE;
    }

    (*printers_amount)++;
    return EXIT_SUCCESS;
}

int scan_network_printers(printer_t (*printers_ips)[MAX_PRINTERS_AMOUNT],
                          int *printers_amount) {
    FILE *fstream = popen(SCAN_NETWORK_PRINTERS_COMMAND, "r");
    char buff[BUFFER_LENGTH];
    memset(buff, 0, sizeof(buff));   

    if(fstream == NULL) {
        return EXIT_FAILURE;
    }

    while (fgets(buff, sizeof(buff), fstream) != NULL) {
        char ip[PRINTER_IP_LENGTH] = {0, };
        char *ip_start = strchr(buff, '[');
        char *ip_end = strchr(buff, ']');
        strncpy(ip, ip_start + 1, ip_end - ip_start - 1);
        if (add_printer_if_not_duplicate(printers_ips, printers_amount, ip)) {
            return EXIT_FAILURE;
        }
    }

    pclose(fstream)

    return EXIT_SUCCESS;
}

int check_printer_errors(printer_t *printer) {
    FILE *fstream = make_request(printer->ip, PRINTER_ERR_CODE_OID);
    char buff[BUFFER_LENGTH];    
    memset(buff, 0, sizeof(buff));   

    if (fstream == NULL) {
        return EXIT_FAILURE;
    }

    fgets(buff, sizeof(buff), fstream);
    int buff_len = strlen(buff);
    char strcode[5] = {buff[buff_len - 7], buff[buff_len - 6],
                       buff[buff_len - 4], buff[buff_len - 3],
                       '\0'};
    int hex_code;
    sscanf(strcode, "%x", &hex_code);

    // Here is a tricky part. Error code contains 4 hex numbers = 16 bits.
    // Each bit is responsible for specific error. For example: 16th bit is responsible for "low paper" error.
    // But it is obvious that printer may have multiple errors for same time.
    // So thats why we need to use "&" operation to detect all of the errors
    for (int err = 0; err < ERROR_CODES_AMOUNT; ++err) {
        if (hex_code & ERROR_CODES[err]) {
            syslog(LOG_ERR, ERROR_LOGS_FORMAT[err], printer->name, printer->ip);
        }
    }

    pclose(fstream)

    return EXIT_SUCCESS;
}

int check_printer_resources(printer_t *printer) {
    if (get_resources_current_level(printer)) {
        return EXIT_FAILURE;
    }

    for (int i = 0; i < printer->resources_amount; ++i) {
        double current_level = (double) printer->resources[i].current / printer->resources[i].max;
        if (current_level <= DANGER_RESOURCE_LEVEL) {
            syslog(LOG_WARN, DANGER_RESOURCE_LEVEL_FORMAT,
                   current_level * 100, printer->resources[i].name, printer->name, printer->ip);
        }
    }
}

void check_printers_state(printer_t printers[MAX_PRINTERS_AMOUNT], int printers_amount) {
    for (int i = 0; i < printers_amount; ++i) {
        if (check_printer_errors(printers + i) ||
            check_printer_resources(printers + i)) {
            return;
        }
    }

    return;
}