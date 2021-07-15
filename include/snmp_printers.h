#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>


#define LOW_PAPER_CODE              0x8000
#define NO_PAPER_CODE               0x4000
#define LOW_TONER_CODE              0x2000
#define NO_TONER_CODE               0x1000
#define DOOR_OPENED_CODE            0x0800
#define JAMMED_CODE                 0x0400
#define OFFLINE_CODE                0x0200
#define SERVICE_REQUESTED_CODE      0x0100
#define INPUT_TRAY_MISSING_CODE     0x0080
#define OUTPUT_TRAY_MISSING_CODE    0x0040
#define MARKER_SUPPLY_MISSING_CODE  0x0020
#define OUTPUT_NEAR_FULL_CODE       0x0010
#define OUTPUT_FULL_CODE            0x0008
#define INPUT_TRAY_EMPTY_CODE       0x0004
#define OVERDUE_PREVENT_MAINT_CODE  0x0002
#define NOT_USED_CODE               0x0001

#define ERROR_CODES_AMOUNT       16
#define ERROR_LOGS_FORMAT_LENGTH 128

#define MAX_PRINTERS_AMOUNT              128
#define MAX_RESOURCES_AMOUNT             128
#define PRINTER_IP_LENGTH                16
#define PRINTER_NAME_MAX_LENGTH          256
#define PRINTER_RESOURCE_NAME_MAX_LENGTH 256
#define REQUEST_STRING_LENGTH            256
#define BUFFER_LENGTH                    1024

#define DANGER_RESOURCE_LEVEL               0.15

#define SCAN_NETWORK_PRINTERS_COMMAND "avahi-browse --all -t -r | grep address"
#define REQUEST_START                 "snmpwalk -v2c -c public"
#define PRINTER_STRING_PREFIX         "STRING: "
#define PRINTER_INTEGER_PREFIX        "INTEGER: "

#define PRINTER_NAME_OID             "1.3.6.1.2.1.25.3.2.1.3.1"
#define PRINTER_ERR_CODE_OID         "1.3.6.1.2.1.25.3.5.1.2"
#define PRINTER_RESOURCES_NAMES      "1.3.6.1.2.1.43.11.1.1.6"
#define PRINTER_RESOURCES_MAX_LEVEL  "1.3.6.1.2.1.43.11.1.1.8"
#define PRINTER_RESOURCES_CURR_LEVEL "1.3.6.1.2.1.43.11.1.1.9"

static const int ERROR_CODES[ERROR_CODES_AMOUNT] = {
    LOW_PAPER_CODE, NO_PAPER_CODE, LOW_TONER_CODE, NO_TONER_CODE,
    DOOR_OPENED_CODE, JAMMED_CODE, OFFLINE_CODE, SERVICE_REQUESTED_CODE,
    INPUT_TRAY_MISSING_CODE, OUTPUT_TRAY_MISSING_CODE, MARKER_SUPPLY_MISSING_CODE, OUTPUT_NEAR_FULL_CODE,
    OUTPUT_FULL_CODE, INPUT_TRAY_EMPTY_CODE, OVERDUE_PREVENT_MAINT_CODE, NOT_USED_CODE
};

static const char ERROR_LOGS_FORMAT[ERROR_CODES_AMOUNT][ERROR_LOGS_FORMAT_LENGTH] =
{
    "Printer has low paper (printer: %s ; ip: %s)",
    "Printer has no paper (printer: %s ; ip: %s)",
    "Printer has low toner (printer: %s ; ip: %s)",
    "Printer has no toner (printer: %s ; ip: %s)",
    "Printer`s door is opened (printer: %s ; ip: %s)",
    "Printer is jammed (printer: %s ; ip: %s)",
    "Printer is offline (printer: %s ; ip: %s)",
    "Printer is requesting service (printer: %s ; ip: %s)",
    "Printer`s input tray is missing (printer: %s ; ip: %s)",
    "Printer`s output tray is missing (printer: %s ; ip: %s)",
    "Printer os missing marker supply (printer: %s ; ip: %s)",
    "Printer`s output is near full (printer: %s ; ip: %s)",
    "Printer`s output is full (printer: %s ; ip: %s)",
    "Printer`s input tray is empty (printer: %s ; ip: %s)",
    "Printer overdue prevent maint (printer: %s ; ip: %s)",
    "Printer is not used (printer: %s ; ip: %s)",
};

static const char DANGER_RESOURCE_LEVEL_FORMAT[] = "Critical level (%.2lf %%) of resource: %s (printer: %s ; ip: %s)";

typedef struct resource_t
{
    char name[PRINTER_RESOURCE_NAME_MAX_LENGTH];
    int current;
    int max;
} resource_t;

typedef struct printer_t
{
    char ip[PRINTER_IP_LENGTH];
    char name[PRINTER_NAME_MAX_LENGTH];
    resource_t resources[MAX_RESOURCES_AMOUNT];
    int resources_amount;
} printer_t;

void  create_request_string        (char [REQUEST_STRING_LENGTH], char *, char *);
FILE* make_request                 (char *, char *);
int   get_printer_name             (printer_t *);
int   get_printer_resources_names  (printer_t *printer);
int   get_resources_max_level      (printer_t *printer);
int   get_resources_current_level  (printer_t *printer);
int   add_printer_if_not_duplicate (printer_t (*)[MAX_PRINTERS_AMOUNT], int *, char [PRINTER_IP_LENGTH]);
int   scan_network_printers        (printer_t (*)[MAX_PRINTERS_AMOUNT],int *);
int   check_printer_errors         (printer_t *printer);
int   check_printer_resources      (printer_t *);
void  check_printers_state         (printer_t [MAX_PRINTERS_AMOUNT], int);