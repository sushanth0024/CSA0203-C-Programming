/*
================================================================================
  WEB-BASED HOTEL MANAGEMENT SYSTEM - BACKEND HTTP SERVER
================================================================================
  File:     backend/hotel_server.c
  Port:     8080 (127.0.0.1)
  Storage:  Binary Files (backend/rooms.dat, backend/bookings.dat, backend/bills.dat)
================================================================================
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef int socklen_t;
#else
    #include <unistd.h>
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #define SOCKET int
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    #define closesocket(s) close(s)
#endif

/* ============================================================================
   CONSTANTS AND CONFIGURATION
   ============================================================================ */
#define SERVER_PORT 8080
#define SERVER_HOST "127.0.0.1"
#define BUFFER_SIZE 65536

#define ROOMS_FILE    "backend/rooms.dat"
#define BOOKINGS_FILE "backend/bookings.dat"
#define BILLS_FILE    "backend/bills.dat"

/* Room Status Constants */
#define ROOM_AVAILABLE 0
#define ROOM_RESERVED  1
#define ROOM_OCCUPIED  2

/* Booking Status Constants */
#define BOOKING_RESERVED   1
#define BOOKING_CHECKED_IN  2
#define BOOKING_CHECKED_OUT 3
#define BOOKING_CANCELLED   4

/* ============================================================================
   DATA STRUCTURES
   ============================================================================ */

/* Room Record Structure */
typedef struct
{
    int room_no;
    char type[20];
    float price;
    int status; /* 0 = Available, 1 = Reserved, 2 = Occupied */
} Room;

/* Booking Record Structure */
typedef struct
{
    int booking_id;
    char customer_name[50];
    char phone[15];
    char email[60];
    int room_no;
    int days;
    char check_in_date[15];
    char check_out_date[15];
    int guests;
    int status; /* 1 = Reserved, 2 = Checked-In, 3 = Checked-Out, 4 = Cancelled */
} Booking;

/* Bill Record Structure */
typedef struct
{
    int booking_id;
    float room_charge;
    float food_charge;
    float service_charge;
    float tax;
    float total;
    int paid; /* 0 = Unpaid, 1 = Paid */
} Bill;

/* ============================================================================
   HELPER PROTOTYPES
   ============================================================================ */
void initializeRooms(void);
int roomExists(int room_no);
int getRoom(int room_no, Room *room);
int updateRoom(Room updated_room);
int generateBookingID(void);
int getBooking(int booking_id, Booking *booking);
int updateBooking(Booking updated_booking);
int getBill(int booking_id, Bill *bill);
int saveBill(Bill bill);

int parseDateToDays(const char *date_str);
int calculateDurationDays(const char *in_date, const char *out_date);

void getJsonStringValue(const char *json, const char *key, char *output, int max_len);
int getJsonIntValue(const char *json, const char *key, int default_val);
float getJsonFloatValue(const char *json, const char *key, float default_val);

void sendHttpResponse(SOCKET client_fd, int status_code, const char *content_type, const char *body);
void sendCorsResponse(SOCKET client_fd);
void serveStaticFile(SOCKET client_fd, const char *path);
void handleClientRequest(SOCKET client_fd, const char *request);

/* ============================================================================
   MAIN SERVER LOOP
   ============================================================================ */
int main(void)
{
    printf("========================================================\n");
    printf("  WEB-BASED HOTEL MANAGEMENT SYSTEM - C BACKEND SERVER  \n");
    printf("========================================================\n");

#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        printf("[ERROR] WSAStartup failed!\n");
        return 1;
    }
#endif

    /* Initialize default room database if missing */
    initializeRooms();

    SOCKET server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == INVALID_SOCKET)
    {
        printf("[ERROR] Failed to create socket!\n");
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    /* Allow address reuse */
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt));

    struct sockaddr_in address;
    memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(SERVER_HOST);
    address.sin_port = htons(SERVER_PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == SOCKET_ERROR)
    {
        printf("[ERROR] Bind failed on %s:%d!\n", SERVER_HOST, SERVER_PORT);
        closesocket(server_fd);
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    if (listen(server_fd, 10) == SOCKET_ERROR)
    {
        printf("[ERROR] Listen failed!\n");
        closesocket(server_fd);
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    printf("[SUCCESS] C Web Server listening on http://%s:%d\n", SERVER_HOST, SERVER_PORT);
    printf("Ready to serve static files from /frontend and REST APIs...\n");
    printf("--------------------------------------------------------\n");

    while (1)
    {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        SOCKET client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);

        if (client_fd == INVALID_SOCKET)
        {
            continue;
        }

        char buffer[BUFFER_SIZE];
        memset(buffer, 0, sizeof(buffer));
        int bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

        if (bytes_received > 0)
        {
            buffer[bytes_received] = '\0';
            handleClientRequest(client_fd, buffer);
        }

        closesocket(client_fd);
    }

    closesocket(server_fd);
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}

/* ============================================================================
   BINARY FILE PERSISTENCE HELPERS
   ============================================================================ */

void initializeRooms(void)
{
    FILE *fp = fopen(ROOMS_FILE, "rb");
    if (fp != NULL)
    {
        fclose(fp);
        return; /* Database already exists */
    }

    Room default_rooms[] = {
        {101, "Single", 1500.00f, ROOM_AVAILABLE},
        {102, "Single", 1500.00f, ROOM_AVAILABLE},
        {201, "Double", 2500.00f, ROOM_AVAILABLE},
        {202, "Double", 2500.00f, ROOM_AVAILABLE},
        {301, "Deluxe", 4000.00f, ROOM_AVAILABLE}
    };

    int count = sizeof(default_rooms) / sizeof(default_rooms[0]);

    fp = fopen(ROOMS_FILE, "wb");
    if (fp == NULL)
    {
        printf("[ERROR] Unable to create %s!\n", ROOMS_FILE);
        return;
    }

    fwrite(default_rooms, sizeof(Room), count, fp);
    fclose(fp);
    printf("[INIT] Created %s with 5 default sample rooms.\n", ROOMS_FILE);
}

int roomExists(int room_no)
{
    Room r;
    return getRoom(room_no, &r);
}

int getRoom(int room_no, Room *room)
{
    FILE *fp = fopen(ROOMS_FILE, "rb");
    if (!fp) return 0;

    Room temp;
    while (fread(&temp, sizeof(Room), 1, fp) == 1)
    {
        if (temp.room_no == room_no)
        {
            if (room) *room = temp;
            fclose(fp);
            return 1;
        }
    }
    fclose(fp);
    return 0;
}

int updateRoom(Room updated_room)
{
    FILE *fp = fopen(ROOMS_FILE, "rb");
    if (!fp) return 0;

    FILE *temp_fp = fopen("backend/temp_rooms.dat", "wb");
    if (!temp_fp)
    {
        fclose(fp);
        return 0;
    }

    Room temp;
    int updated = 0;
    while (fread(&temp, sizeof(Room), 1, fp) == 1)
    {
        if (temp.room_no == updated_room.room_no)
        {
            fwrite(&updated_room, sizeof(Room), 1, temp_fp);
            updated = 1;
        }
        else
        {
            fwrite(&temp, sizeof(Room), 1, temp_fp);
        }
    }
    fclose(fp);
    fclose(temp_fp);

    remove(ROOMS_FILE);
    rename("backend/temp_rooms.dat", ROOMS_FILE);
    return updated;
}

int generateBookingID(void)
{
    FILE *fp = fopen(BOOKINGS_FILE, "rb");
    if (!fp) return 1001;

    Booking temp;
    int max_id = 1000;
    while (fread(&temp, sizeof(Booking), 1, fp) == 1)
    {
        if (temp.booking_id > max_id)
        {
            max_id = temp.booking_id;
        }
    }
    fclose(fp);
    return max_id + 1;
}

int getBooking(int booking_id, Booking *booking)
{
    FILE *fp = fopen(BOOKINGS_FILE, "rb");
    if (!fp) return 0;

    Booking temp;
    while (fread(&temp, sizeof(Booking), 1, fp) == 1)
    {
        if (temp.booking_id == booking_id)
        {
            if (booking) *booking = temp;
            fclose(fp);
            return 1;
        }
    }
    fclose(fp);
    return 0;
}

int updateBooking(Booking updated_booking)
{
    FILE *fp = fopen(BOOKINGS_FILE, "rb");
    if (!fp) return 0;

    FILE *temp_fp = fopen("backend/temp_bookings.dat", "wb");
    if (!temp_fp)
    {
        fclose(fp);
        return 0;
    }

    Booking temp;
    int updated = 0;
    while (fread(&temp, sizeof(Booking), 1, fp) == 1)
    {
        if (temp.booking_id == updated_booking.booking_id)
        {
            fwrite(&updated_booking, sizeof(Booking), 1, temp_fp);
            updated = 1;
        }
        else
        {
            fwrite(&temp, sizeof(Booking), 1, temp_fp);
        }
    }
    fclose(fp);
    fclose(temp_fp);

    remove(BOOKINGS_FILE);
    rename("backend/temp_bookings.dat", BOOKINGS_FILE);
    return updated;
}

int getBill(int booking_id, Bill *bill)
{
    FILE *fp = fopen(BILLS_FILE, "rb");
    if (!fp) return 0;

    Bill temp;
    while (fread(&temp, sizeof(Bill), 1, fp) == 1)
    {
        if (temp.booking_id == booking_id)
        {
            if (bill) *bill = temp;
            fclose(fp);
            return 1;
        }
    }
    fclose(fp);
    return 0;
}

int saveBill(Bill new_bill)
{
    FILE *fp = fopen(BILLS_FILE, "rb");
    FILE *temp_fp = fopen("backend/temp_bills.dat", "wb");
    int updated = 0;

    if (fp && temp_fp)
    {
        Bill temp;
        while (fread(&temp, sizeof(Bill), 1, fp) == 1)
        {
            if (temp.booking_id == new_bill.booking_id)
            {
                fwrite(&new_bill, sizeof(Bill), 1, temp_fp);
                updated = 1;
            }
            else
            {
                fwrite(&temp, sizeof(Bill), 1, temp_fp);
            }
        }
        fclose(fp);
        fclose(temp_fp);
        remove(BILLS_FILE);
        rename("backend/temp_bills.dat", BILLS_FILE);
    }
    else
    {
        if (fp) fclose(fp);
        if (temp_fp) fclose(temp_fp);
    }

    if (!updated)
    {
        FILE *append_fp = fopen(BILLS_FILE, "ab");
        if (!append_fp) return 0;
        fwrite(&new_bill, sizeof(Bill), 1, append_fp);
        fclose(append_fp);
    }
    return 1;
}

/* ============================================================================
   DATE CALCULATION LOGIC IN C
   ============================================================================ */

int parseDateToDays(const char *date_str)
{
    int day = 0, month = 0, year = 0;

    if (sscanf(date_str, "%d/%d/%d", &day, &month, &year) == 3)
    {
        /* DD/MM/YYYY format */
    }
    else if (sscanf(date_str, "%d-%d-%d", &year, &month, &day) == 3)
    {
        /* YYYY-MM-DD format */
    }
    else
    {
        return -1;
    }

    struct tm tm_date;
    memset(&tm_date, 0, sizeof(tm_date));
    tm_date.tm_mday = day;
    tm_date.tm_mon = month - 1;
    tm_date.tm_year = year - 1900;

    time_t t = mktime(&tm_date);
    if (t == (time_t)-1) return -1;

    return (int)(t / (24 * 3600));
}

int calculateDurationDays(const char *in_date, const char *out_date)
{
    int in_days = parseDateToDays(in_date);
    int out_days = parseDateToDays(out_date);

    if (in_days < 0 || out_days < 0) return -1;

    int diff = out_days - in_days;
    return (diff > 0) ? diff : -1;
}

/* ============================================================================
   ROBUST JSON PARSER HELPERS FOR C
   ============================================================================ */

void getJsonStringValue(const char *json, const char *key, char *output, int max_len)
{
    output[0] = '\0';
    if (!json || !key) return;

    char search_key[100];
    snprintf(search_key, sizeof(search_key), "\"%s\"", key);

    const char *pos = strstr(json, search_key);
    if (!pos) return;

    pos += strlen(search_key);
    pos = strchr(pos, ':');
    if (!pos) return;
    pos++;

    while (*pos == ' ' || *pos == '\t' || *pos == '\r' || *pos == '\n') pos++;

    if (*pos == '"')
    {
        pos++;
        const char *end_pos = strchr(pos, '"');
        if (!end_pos) return;
        int len = (int)(end_pos - pos);
        if (len >= max_len) len = max_len - 1;
        strncpy(output, pos, len);
        output[len] = '\0';
    }
}

int getJsonIntValue(const char *json, const char *key, int default_val)
{
    if (!json || !key) return default_val;

    char search_key[100];
    snprintf(search_key, sizeof(search_key), "\"%s\"", key);

    const char *pos = strstr(json, search_key);
    if (!pos) return default_val;

    pos += strlen(search_key);
    pos = strchr(pos, ':');
    if (!pos) return default_val;
    pos++;

    while (*pos == ' ' || *pos == '\t' || *pos == '\r' || *pos == '\n' || *pos == '"') pos++;

    int val = default_val;
    if (sscanf(pos, "%d", &val) == 1)
    {
        return val;
    }
    return default_val;
}

float getJsonFloatValue(const char *json, const char *key, float default_val)
{
    if (!json || !key) return default_val;

    char search_key[100];
    snprintf(search_key, sizeof(search_key), "\"%s\"", key);

    const char *pos = strstr(json, search_key);
    if (!pos) return default_val;

    pos += strlen(search_key);
    pos = strchr(pos, ':');
    if (!pos) return default_val;
    pos++;

    while (*pos == ' ' || *pos == '\t' || *pos == '\r' || *pos == '\n' || *pos == '"') pos++;

    float val = default_val;
    if (sscanf(pos, "%f", &val) == 1)
    {
        return val;
    }
    return default_val;
}

/* ============================================================================
   HTTP RESPONSE & STATIC FILE SERVING HELPERS
   ============================================================================ */

void sendHttpResponse(SOCKET client_fd, int status_code, const char *content_type, const char *body)
{
    const char *status_text = "OK";
    if (status_code == 201) status_text = "Created";
    else if (status_code == 400) status_text = "Bad Request";
    else if (status_code == 403) status_text = "Forbidden";
    else if (status_code == 404) status_text = "Not Found";
    else if (status_code == 409) status_text = "Conflict";
    else if (status_code == 500) status_text = "Internal Server Error";

    int body_len = body ? (int)strlen(body) : 0;

    char header[1024];
    snprintf(header, sizeof(header),
             "HTTP/1.1 %d %s\r\n"
             "Content-Type: %s\r\n"
             "Content-Length: %d\r\n"
             "Access-Control-Allow-Origin: *\r\n"
             "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
             "Access-Control-Allow-Headers: Content-Type\r\n"
             "Connection: close\r\n\r\n",
             status_code, status_text, content_type, body_len);

    send(client_fd, header, (int)strlen(header), 0);
    if (body_len > 0)
    {
        send(client_fd, body, body_len, 0);
    }
}

void sendCorsResponse(SOCKET client_fd)
{
    sendHttpResponse(client_fd, 200, "text/plain", "");
}

/* Static File Host with Exact Path Traversal Protection */
void serveStaticFile(SOCKET client_fd, const char *url_path)
{
    /* Path Traversal & Directory Guard */
    if (strstr(url_path, "..") != NULL ||
        strstr(url_path, ".dat") != NULL ||
        strncmp(url_path, "/backend", 8) == 0)
    {
        sendHttpResponse(client_fd, 403, "application/json", "{\"success\":false,\"message\":\"Access Denied\"}");
        return;
    }

    /* Guard against serving source/executable files */
    const char *dot = strrchr(url_path, '.');
    if (dot && (strcmp(dot, ".c") == 0 || strcmp(dot, ".exe") == 0 || strcmp(dot, ".o") == 0))
    {
        sendHttpResponse(client_fd, 403, "application/json", "{\"success\":false,\"message\":\"Access Denied\"}");
        return;
    }

    char file_path[512];
    if (strcmp(url_path, "/") == 0 || strcmp(url_path, "/index.html") == 0)
    {
        snprintf(file_path, sizeof(file_path), "frontend/index.html");
    }
    else
    {
        snprintf(file_path, sizeof(file_path), "frontend%s", url_path);
    }

    FILE *fp = fopen(file_path, "rb");
    if (!fp)
    {
        sendHttpResponse(client_fd, 404, "application/json", "{\"success\":false,\"message\":\"File Not Found\"}");
        return;
    }

    /* Determine MIME type */
    const char *content_type = "text/html";
    if (strstr(file_path, ".css")) content_type = "text/css";
    else if (strstr(file_path, ".js")) content_type = "application/javascript";
    else if (strstr(file_path, ".json")) content_type = "application/json";
    else if (strstr(file_path, ".png")) content_type = "image/png";
    else if (strstr(file_path, ".jpg") || strstr(file_path, ".jpeg")) content_type = "image/jpeg";

    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *file_buffer = (char *)malloc(file_size + 1);
    if (!file_buffer)
    {
        fclose(fp);
        sendHttpResponse(client_fd, 500, "text/plain", "Memory Allocation Error");
        return;
    }

    fread(file_buffer, 1, file_size, fp);
    fclose(fp);
    file_buffer[file_size] = '\0';

    char header[1024];
    snprintf(header, sizeof(header),
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: %s\r\n"
             "Content-Length: %ld\r\n"
             "Connection: close\r\n\r\n",
             content_type, file_size);

    send(client_fd, header, (int)strlen(header), 0);
    send(client_fd, file_buffer, file_size, 0);

    free(file_buffer);
}

/* ============================================================================
   RESTFUL API ROUTER & CONTROLLER HANDLERS
   ============================================================================ */

void handleClientRequest(SOCKET client_fd, const char *request)
{
    char method[16], path[256], protocol[16];
    if (sscanf(request, "%s %s %s", method, path, protocol) < 2)
    {
        sendHttpResponse(client_fd, 400, "application/json", "{\"success\":false,\"message\":\"Invalid Request\"}");
        return;
    }

    /* Handle CORS Pre-flight */
    if (strcmp(method, "OPTIONS") == 0)
    {
        sendCorsResponse(client_fd);
        return;
    }

    /* Extract Request Body supporting both \r\n\r\n and \n\n */
    const char *body = strstr(request, "\r\n\r\n");
    if (body)
    {
        body += 4;
    }
    else
    {
        body = strstr(request, "\n\n");
        if (body) body += 2;
        else body = "";
    }

    /* ------------------------------------------------------------------------
       1. GET /api/rooms - Return list of all rooms
       ------------------------------------------------------------------------ */
    if (strcmp(method, "GET") == 0 && strcmp(path, "/api/rooms") == 0)
    {
        FILE *fp = fopen(ROOMS_FILE, "rb");
        char json[BUFFER_SIZE];
        strcpy(json, "{\"success\":true,\"rooms\":[");

        if (fp)
        {
            Room r;
            int first = 1;
            while (fread(&r, sizeof(Room), 1, fp) == 1)
            {
                char item[256];
                const char *status_str = (r.status == 0) ? "Available" : (r.status == 1) ? "Reserved" : "Occupied";
                snprintf(item, sizeof(item), "%s{\"room_no\":%d,\"type\":\"%s\",\"price\":%.2f,\"status\":%d,\"status_str\":\"%s\"}",
                         first ? "" : ",", r.room_no, r.type, r.price, r.status, status_str);
                strcat(json, item);
                first = 0;
            }
            fclose(fp);
        }
        strcat(json, "]}");
        sendHttpResponse(client_fd, 200, "application/json", json);
        return;
    }

    /* ------------------------------------------------------------------------
       2. GET /api/rooms/available - Return list of available rooms only
       ------------------------------------------------------------------------ */
    if (strcmp(method, "GET") == 0 && strcmp(path, "/api/rooms/available") == 0)
    {
        FILE *fp = fopen(ROOMS_FILE, "rb");
        char json[BUFFER_SIZE];
        strcpy(json, "{\"success\":true,\"rooms\":[");

        if (fp)
        {
            Room r;
            int first = 1;
            while (fread(&r, sizeof(Room), 1, fp) == 1)
            {
                if (r.status == ROOM_AVAILABLE)
                {
                    char item[256];
                    snprintf(item, sizeof(item), "%s{\"room_no\":%d,\"type\":\"%s\",\"price\":%.2f,\"status\":0,\"status_str\":\"Available\"}",
                             first ? "" : ",", r.room_no, r.type, r.price);
                    strcat(json, item);
                    first = 0;
                }
            }
            fclose(fp);
        }
        strcat(json, "]}");
        sendHttpResponse(client_fd, 200, "application/json", json);
        return;
    }

    /* ------------------------------------------------------------------------
       3. POST /api/rooms/add - Add a new room (admin)
       ------------------------------------------------------------------------ */
    if (strcmp(method, "POST") == 0 && strcmp(path, "/api/rooms/add") == 0)
    {
        int room_no = getJsonIntValue(body, "room_no", 0);
        char type[20];
        getJsonStringValue(body, "type", type, sizeof(type));
        float price = getJsonFloatValue(body, "price", 0.0f);

        if (room_no <= 0 || strlen(type) == 0 || price <= 0.0f)
        {
            sendHttpResponse(client_fd, 400, "application/json", "{\"success\":false,\"message\":\"Invalid room details! Price and Room number must be > 0.\"}");
            return;
        }

        if (roomExists(room_no))
        {
            sendHttpResponse(client_fd, 409, "application/json", "{\"success\":false,\"message\":\"Room number already exists!\"}");
            return;
        }

        Room r;
        r.room_no = room_no;
        strcpy(r.type, type);
        r.price = price;
        r.status = ROOM_AVAILABLE;

        FILE *fp = fopen(ROOMS_FILE, "ab");
        if (!fp)
        {
            sendHttpResponse(client_fd, 500, "application/json", "{\"success\":false,\"message\":\"Failed to open room database.\"}");
            return;
        }
        fwrite(&r, sizeof(Room), 1, fp);
        fclose(fp);

        sendHttpResponse(client_fd, 201, "application/json", "{\"success\":true,\"message\":\"Room added successfully!\"}");
        return;
    }

    /* ------------------------------------------------------------------------
       4. POST /api/bookings - Create new customer reservation
       ------------------------------------------------------------------------ */
    if (strcmp(method, "POST") == 0 && strcmp(path, "/api/bookings") == 0)
    {
        Booking b;
        getJsonStringValue(body, "customer_name", b.customer_name, sizeof(b.customer_name));
        getJsonStringValue(body, "phone", b.phone, sizeof(b.phone));
        getJsonStringValue(body, "email", b.email, sizeof(b.email));
        getJsonStringValue(body, "check_in_date", b.check_in_date, sizeof(b.check_in_date));
        getJsonStringValue(body, "check_out_date", b.check_out_date, sizeof(b.check_out_date));
        b.room_no = getJsonIntValue(body, "room_no", 0);
        b.guests = getJsonIntValue(body, "guests", 1);

        if (strlen(b.customer_name) == 0 || strlen(b.phone) == 0 || b.room_no <= 0)
        {
            sendHttpResponse(client_fd, 400, "application/json", "{\"success\":false,\"message\":\"Missing required customer details!\"}");
            return;
        }

        Room r;
        if (!getRoom(b.room_no, &r))
        {
            sendHttpResponse(client_fd, 404, "application/json", "{\"success\":false,\"message\":\"Selected room not found!\"}");
            return;
        }

        if (r.status != ROOM_AVAILABLE)
        {
            sendHttpResponse(client_fd, 409, "application/json", "{\"success\":false,\"message\":\"Selected room is no longer available!\"}");
            return;
        }

        /* Calculate days in C backend */
        int days = calculateDurationDays(b.check_in_date, b.check_out_date);
        if (days <= 0)
        {
            sendHttpResponse(client_fd, 400, "application/json", "{\"success\":false,\"message\":\"Invalid dates! Check-out must be after Check-in.\"}");
            return;
        }
        b.days = days;
        b.booking_id = generateBookingID();
        b.status = BOOKING_RESERVED;

        FILE *fp = fopen(BOOKINGS_FILE, "ab");
        if (!fp)
        {
            sendHttpResponse(client_fd, 500, "application/json", "{\"success\":false,\"message\":\"Failed to save booking record.\"}");
            return;
        }
        fwrite(&b, sizeof(Booking), 1, fp);
        fclose(fp);

        /* Update Room Status to Reserved */
        r.status = ROOM_RESERVED;
        updateRoom(r);

        float room_charge = r.price * b.days;

        char json_res[512];
        snprintf(json_res, sizeof(json_res),
                 "{\"success\":true,\"booking_id\":%d,\"customer_name\":\"%s\",\"room_no\":%d,\"room_type\":\"%s\",\"days\":%d,\"room_charge\":%.2f,\"message\":\"Booking confirmed successfully!\"}",
                 b.booking_id, b.customer_name, b.room_no, r.type, b.days, room_charge);

        sendHttpResponse(client_fd, 201, "application/json", json_res);
        return;
    }

    /* ------------------------------------------------------------------------
       5. POST /api/bookings/lookup - Customer lookup by ID + Phone
       ------------------------------------------------------------------------ */
    if (strcmp(method, "POST") == 0 && strcmp(path, "/api/bookings/lookup") == 0)
    {
        int booking_id = getJsonIntValue(body, "booking_id", 0);
        char phone[15];
        getJsonStringValue(body, "phone", phone, sizeof(phone));

        Booking b;
        if (!getBooking(booking_id, &b) || strcmp(b.phone, phone) != 0)
        {
            sendHttpResponse(client_fd, 404, "application/json", "{\"success\":false,\"message\":\"Booking not found! Please check Booking ID and Phone Number.\"}");
            return;
        }

        Room r;
        getRoom(b.room_no, &r);

        const char *status_str = (b.status == 1) ? "RESERVED" : (b.status == 2) ? "CHECKED-IN" : (b.status == 3) ? "CHECKED-OUT" : "CANCELLED";

        Bill bill;
        int bill_exists = getBill(b.booking_id, &bill);
        float total_billed = bill_exists ? bill.total : (r.price * b.days);

        char json_res[1024];
        snprintf(json_res, sizeof(json_res),
                 "{\"success\":true,\"booking\":{\"booking_id\":%d,\"customer_name\":\"%s\",\"phone\":\"%s\",\"email\":\"%s\",\"room_no\":%d,\"room_type\":\"%s\",\"days\":%d,\"check_in\":\"%s\",\"check_out\":\"%s\",\"guests\":%d,\"status\":%d,\"status_str\":\"%s\",\"total_billed\":%.2f}}",
                 b.booking_id, b.customer_name, b.phone, b.email, b.room_no, r.type, b.days, b.check_in_date, b.check_out_date, b.guests, b.status, status_str, total_billed);

        sendHttpResponse(client_fd, 200, "application/json", json_res);
        return;
    }

    /* ------------------------------------------------------------------------
       6. POST /api/bookings/cancel - Customer cancel reservation
       ------------------------------------------------------------------------ */
    if (strcmp(method, "POST") == 0 && strcmp(path, "/api/bookings/cancel") == 0)
    {
        int booking_id = getJsonIntValue(body, "booking_id", 0);
        char phone[15];
        getJsonStringValue(body, "phone", phone, sizeof(phone));

        Booking b;
        if (!getBooking(booking_id, &b) || strcmp(b.phone, phone) != 0)
        {
            sendHttpResponse(client_fd, 404, "application/json", "{\"success\":false,\"message\":\"Booking not found! Check Booking ID and Phone Number.\"}");
            return;
        }

        if (b.status != BOOKING_RESERVED)
        {
            sendHttpResponse(client_fd, 400, "application/json", "{\"success\":false,\"message\":\"Only reservations with status 'RESERVED' can be cancelled!\"}");
            return;
        }

        /* Update booking -> Cancelled */
        b.status = BOOKING_CANCELLED;
        updateBooking(b);

        /* Update room -> Available */
        Room r;
        if (getRoom(b.room_no, &r))
        {
            r.status = ROOM_AVAILABLE;
            updateRoom(r);
        }

        sendHttpResponse(client_fd, 200, "application/json", "{\"success\":true,\"message\":\"Reservation cancelled successfully!\"}");
        return;
    }

    /* ------------------------------------------------------------------------
       7. GET /api/bookings - Fetch all bookings (admin)
       ------------------------------------------------------------------------ */
    if (strcmp(method, "GET") == 0 && strcmp(path, "/api/bookings") == 0)
    {
        FILE *fp = fopen(BOOKINGS_FILE, "rb");
        char json[BUFFER_SIZE];
        strcpy(json, "{\"success\":true,\"bookings\":[");

        if (fp)
        {
            Booking b;
            int first = 1;
            while (fread(&b, sizeof(Booking), 1, fp) == 1)
            {
                char item[512];
                const char *status_str = (b.status == 1) ? "RESERVED" : (b.status == 2) ? "CHECKED-IN" : (b.status == 3) ? "CHECKED-OUT" : "CANCELLED";
                snprintf(item, sizeof(item),
                         "%s{\"booking_id\":%d,\"customer_name\":\"%s\",\"phone\":\"%s\",\"email\":\"%s\",\"room_no\":%d,\"days\":%d,\"check_in\":\"%s\",\"check_out\":\"%s\",\"guests\":%d,\"status\":%d,\"status_str\":\"%s\"}",
                         first ? "" : ",", b.booking_id, b.customer_name, b.phone, b.email, b.room_no, b.days, b.check_in_date, b.check_out_date, b.guests, b.status, status_str);
                strcat(json, item);
                first = 0;
            }
            fclose(fp);
        }
        strcat(json, "]}");
        sendHttpResponse(client_fd, 200, "application/json", json);
        return;
    }

    /* ------------------------------------------------------------------------
       8. POST /api/checkin - Admin Check-In customer
       ------------------------------------------------------------------------ */
    if (strcmp(method, "POST") == 0 && strcmp(path, "/api/checkin") == 0)
    {
        int booking_id = getJsonIntValue(body, "booking_id", 0);

        Booking b;
        if (!getBooking(booking_id, &b))
        {
            sendHttpResponse(client_fd, 404, "application/json", "{\"success\":false,\"message\":\"Booking ID not found!\"}");
            return;
        }

        if (b.status != BOOKING_RESERVED)
        {
            sendHttpResponse(client_fd, 400, "application/json", "{\"success\":false,\"message\":\"Only RESERVED bookings can be checked in!\"}");
            return;
        }

        b.status = BOOKING_CHECKED_IN;
        updateBooking(b);

        Room r;
        if (getRoom(b.room_no, &r))
        {
            r.status = ROOM_OCCUPIED;
            updateRoom(r);
        }

        sendHttpResponse(client_fd, 200, "application/json", "{\"success\":true,\"message\":\"Customer checked in successfully!\"}");
        return;
    }

    /* ------------------------------------------------------------------------
       9. GET /api/bills/{id} & POST /api/bills - Billing Operations
       ------------------------------------------------------------------------ */
    if (strcmp(method, "GET") == 0 && strncmp(path, "/api/bills/", 11) == 0)
    {
        int booking_id = atoi(path + 11);
        Bill bill;
        if (!getBill(booking_id, &bill))
        {
            sendHttpResponse(client_fd, 404, "application/json", "{\"success\":false,\"message\":\"Bill not found for this booking.\"}");
            return;
        }

        char json_res[512];
        snprintf(json_res, sizeof(json_res),
                 "{\"success\":true,\"bill\":{\"booking_id\":%d,\"room_charge\":%.2f,\"food_charge\":%.2f,\"service_charge\":%.2f,\"tax\":%.2f,\"total\":%.2f,\"paid\":%d}}",
                 bill.booking_id, bill.room_charge, bill.food_charge, bill.service_charge, bill.tax, bill.total, bill.paid);
        sendHttpResponse(client_fd, 200, "application/json", json_res);
        return;
    }

    if (strcmp(method, "POST") == 0 && strcmp(path, "/api/bills") == 0)
    {
        int booking_id = getJsonIntValue(body, "booking_id", 0);
        float food_charge = getJsonFloatValue(body, "food_charge", 0.0f);
        float service_charge = getJsonFloatValue(body, "service_charge", 0.0f);

        Booking b;
        if (!getBooking(booking_id, &b))
        {
            sendHttpResponse(client_fd, 404, "application/json", "{\"success\":false,\"message\":\"Booking ID not found!\"}");
            return;
        }

        Room r;
        if (!getRoom(b.room_no, &r))
        {
            sendHttpResponse(client_fd, 404, "application/json", "{\"success\":false,\"message\":\"Room record not found!\"}");
            return;
        }

        float room_charge = r.price * b.days;
        float subtotal = room_charge + food_charge + service_charge;
        float tax = subtotal * 0.10f; /* 10% tax */
        float total = subtotal + tax;

        Bill bill;
        bill.booking_id = booking_id;
        bill.room_charge = room_charge;
        bill.food_charge = food_charge;
        bill.service_charge = service_charge;
        bill.tax = tax;
        bill.total = total;
        bill.paid = 1;

        saveBill(bill);

        char json_res[512];
        snprintf(json_res, sizeof(json_res),
                 "{\"success\":true,\"bill\":{\"booking_id\":%d,\"room_charge\":%.2f,\"food_charge\":%.2f,\"service_charge\":%.2f,\"tax\":%.2f,\"total\":%.2f},\"message\":\"Bill generated successfully!\"}",
                 bill.booking_id, bill.room_charge, bill.food_charge, bill.service_charge, bill.tax, bill.total);
        sendHttpResponse(client_fd, 200, "application/json", json_res);
        return;
    }

    /* ------------------------------------------------------------------------
       10. POST /api/checkout - Admin Check-Out customer
       ------------------------------------------------------------------------ */
    if (strcmp(method, "POST") == 0 && strcmp(path, "/api/checkout") == 0)
    {
        int booking_id = getJsonIntValue(body, "booking_id", 0);

        Booking b;
        if (!getBooking(booking_id, &b))
        {
            sendHttpResponse(client_fd, 404, "application/json", "{\"success\":false,\"message\":\"Booking ID not found!\"}");
            return;
        }

        if (b.status != BOOKING_CHECKED_IN)
        {
            sendHttpResponse(client_fd, 400, "application/json", "{\"success\":false,\"message\":\"Only CHECKED-IN customers can check out!\"}");
            return;
        }

        Bill bill;
        if (!getBill(booking_id, &bill))
        {
            sendHttpResponse(client_fd, 400, "application/json", "{\"success\":false,\"message\":\"Bill has not been generated for this booking yet! Please generate bill first.\"}");
            return;
        }

        b.status = BOOKING_CHECKED_OUT;
        updateBooking(b);

        Room r;
        if (getRoom(b.room_no, &r))
        {
            r.status = ROOM_AVAILABLE;
            updateRoom(r);
        }

        sendHttpResponse(client_fd, 200, "application/json", "{\"success\":true,\"message\":\"Customer checked out successfully! Room is now Available.\"}");
        return;
    }

    /* ------------------------------------------------------------------------
       11. GET /api/stats - Admin Dashboard Statistics
       ------------------------------------------------------------------------ */
    if (strcmp(method, "GET") == 0 && strcmp(path, "/api/stats") == 0)
    {
        int total_rooms = 0, avail_rooms = 0, res_rooms = 0, occ_rooms = 0;
        int total_bookings = 0;
        float total_revenue = 0.0f;

        FILE *fp = fopen(ROOMS_FILE, "rb");
        if (fp)
        {
            Room r;
            while (fread(&r, sizeof(Room), 1, fp) == 1)
            {
                total_rooms++;
                if (r.status == ROOM_AVAILABLE) avail_rooms++;
                else if (r.status == ROOM_RESERVED) res_rooms++;
                else if (r.status == ROOM_OCCUPIED) occ_rooms++;
            }
            fclose(fp);
        }

        fp = fopen(BOOKINGS_FILE, "rb");
        if (fp)
        {
            Booking b;
            while (fread(&b, sizeof(Booking), 1, fp) == 1)
            {
                total_bookings++;
            }
            fclose(fp);
        }

        fp = fopen(BILLS_FILE, "rb");
        if (fp)
        {
            Bill bill;
            while (fread(&bill, sizeof(Bill), 1, fp) == 1)
            {
                total_revenue += bill.total;
            }
            fclose(fp);
        }

        char json_res[512];
        snprintf(json_res, sizeof(json_res),
                 "{\"success\":true,\"stats\":{\"total_rooms\":%d,\"available_rooms\":%d,\"reserved_rooms\":%d,\"occupied_rooms\":%d,\"total_bookings\":%d,\"total_revenue\":%.2f}}",
                 total_rooms, avail_rooms, res_rooms, occ_rooms, total_bookings, total_revenue);
        sendHttpResponse(client_fd, 200, "application/json", json_res);
        return;
    }

    /* Serve Static Frontend Files */
    serveStaticFile(client_fd, path);
}
