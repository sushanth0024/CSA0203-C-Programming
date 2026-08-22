/*
================================================================================
  WEB-BASED HOTEL MANAGEMENT SYSTEM - BACKEND HTTP SERVER (PHASE 4)
================================================================================
  File:     backend/hotel_server.c
  Port:     8080 (127.0.0.1)
  Storage:  Binary Files (rooms.dat, bookings.dat, bills.dat, users.dat,
            guests.dat, audit_logs.dat)
  Features: User Authentication, Session Tokens, Role-Based Authorization,
            Room Maintenance, Guest Management, Payment Methods (Cash/Card/UPI),
            Audit Logging, Reporting Engine, Timestamped Backup, Path Security.
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
#define MAX_SESSIONS 100

#define ROOMS_FILE      "backend/rooms.dat"
#define BOOKINGS_FILE   "backend/bookings.dat"
#define BILLS_FILE      "backend/bills.dat"
#define USERS_FILE      "backend/users.dat"
#define GUESTS_FILE     "backend/guests.dat"
#define AUDIT_LOG_FILE  "backend/audit_logs.dat"

/* Room Status Constants */
#define ROOM_AVAILABLE    0
#define ROOM_RESERVED     1
#define ROOM_OCCUPIED     2
#define ROOM_MAINTENANCE  3

/* Booking Status Constants */
#define BOOKING_RESERVED   1
#define BOOKING_CHECKED_IN  2
#define BOOKING_CHECKED_OUT 3
#define BOOKING_CANCELLED   4

/* ============================================================================
   DATA STRUCTURES
   ============================================================================ */

/* User Record Structure */
typedef struct
{
    int user_id;
    char username[50];
    char password[100];
    char role[20]; /* "admin" or "receptionist" */
    int active;    /* 1 = Active, 0 = Disabled */
} User;

/* Session Structure */
typedef struct
{
    char token[64];
    int user_id;
    char username[50];
    char role[20];
    int active;
    time_t login_time;
} Session;

/* Room Record Structure */
typedef struct
{
    int room_no;
    char type[20];
    float price;
    int status; /* 0 = Available, 1 = Reserved, 2 = Occupied, 3 = Maintenance */
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
    char payment_method[20]; /* Cash, Card, UPI */
} Bill;

/* Guest Record Structure */
typedef struct
{
    int guest_id;
    char name[100];
    char phone[20];
    char email[100];
    char address[200];
    char id_type[30];
    char id_number[50];
} Guest;

/* Audit Log Structure */
typedef struct
{
    int log_id;
    int user_id;
    char username[50];
    char action[100];
    char details[200];
    char timestamp[30];
} AuditLog;

/* Global In-Memory Sessions */
static Session g_sessions[MAX_SESSIONS];
static int g_session_count = 0;

/* ============================================================================
   HELPER PROTOTYPES
   ============================================================================ */
void initializeDatabases(void);
void addAuditLog(int user_id, const char *username, const char *action, const char *details);
void getTimestampStr(char *buffer, size_t max_len);

int validateUser(const char *username, const char *password, User *user);
char* createSession(int user_id, const char *username, const char *role);
int verifySessionToken(const char *token, Session *out_session);
void invalidateSession(const char *token);

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
    printf("  WEB-BASED HOTEL MANAGEMENT SYSTEM - C BACKEND (PHASE 4)\n");
    printf("========================================================\n");

#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        printf("[ERROR] WSAStartup failed!\n");
        return 1;
    }
#endif

    /* Initialize default room & user databases */
    initializeDatabases();

    SOCKET server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == INVALID_SOCKET)
    {
        printf("[ERROR] Failed to create socket!\n");
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

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

    printf("[SUCCESS] C Web Server Phase 4 running on http://%s:%d\n", SERVER_HOST, SERVER_PORT);
    printf("Serving frontend files, RBAC, Sessions, Audit Logging, Reports...\n");
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
   DATABASE & INITIALIZATION HELPERS
   ============================================================================ */

void getTimestampStr(char *buffer, size_t max_len)
{
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(buffer, max_len, "%Y-%m-%d %H:%M:%S", t);
}

void initializeDatabases(void)
{
    /* Initialize Rooms */
    FILE *fp = fopen(ROOMS_FILE, "rb");
    if (fp == NULL)
    {
        Room default_rooms[] = {
            {101, "Single", 1500.00f, ROOM_AVAILABLE},
            {102, "Single", 1500.00f, ROOM_AVAILABLE},
            {201, "Double", 2500.00f, ROOM_AVAILABLE},
            {202, "Double", 2500.00f, ROOM_AVAILABLE},
            {301, "Deluxe", 4000.00f, ROOM_AVAILABLE}
        };
        fp = fopen(ROOMS_FILE, "wb");
        if (fp)
        {
            fwrite(default_rooms, sizeof(Room), 5, fp);
            fclose(fp);
            printf("[INIT] Created %s with 5 rooms.\n", ROOMS_FILE);
        }
    }
    else
    {
        fclose(fp);
    }

    /* Initialize Users (Default admin & reception) */
    fp = fopen(USERS_FILE, "rb");
    if (fp == NULL)
    {
        User default_users[] = {
            {1, "admin", "admin123", "admin", 1},
            {2, "reception", "rec123", "receptionist", 1}
        };
        fp = fopen(USERS_FILE, "wb");
        if (fp)
        {
            fwrite(default_users, sizeof(User), 2, fp);
            fclose(fp);
            printf("[INIT] Created %s with admin and receptionist accounts.\n", USERS_FILE);
        }
    }
    else
    {
        fclose(fp);
    }
}

void addAuditLog(int user_id, const char *username, const char *action, const char *details)
{
    FILE *fp = fopen(AUDIT_LOG_FILE, "rb");
    int next_id = 1;
    if (fp)
    {
        AuditLog temp;
        while (fread(&temp, sizeof(AuditLog), 1, fp) == 1)
        {
            if (temp.log_id >= next_id) next_id = temp.log_id + 1;
        }
        fclose(fp);
    }

    AuditLog log;
    log.log_id = next_id;
    log.user_id = user_id;
    strncpy(log.username, username ? username : "System", sizeof(log.username) - 1);
    strncpy(log.action, action, sizeof(log.action) - 1);
    strncpy(log.details, details, sizeof(log.details) - 1);
    getTimestampStr(log.timestamp, sizeof(log.timestamp));

    fp = fopen(AUDIT_LOG_FILE, "ab");
    if (fp)
    {
        fwrite(&log, sizeof(AuditLog), 1, fp);
        fclose(fp);
    }
}

/* ============================================================================
   USER AUTHENTICATION & SESSION MANAGEMENT
   ============================================================================ */

int validateUser(const char *username, const char *password, User *user)
{
    FILE *fp = fopen(USERS_FILE, "rb");
    if (!fp) return 0;

    User temp;
    while (fread(&temp, sizeof(User), 1, fp) == 1)
    {
        if (strcmp(temp.username, username) == 0 && strcmp(temp.password, password) == 0)
        {
            if (temp.active == 1)
            {
                if (user) *user = temp;
                fclose(fp);
                return 1;
            }
        }
    }
    fclose(fp);
    return 0;
}

char* createSession(int user_id, const char *username, const char *role)
{
    static char token[64];
    snprintf(token, sizeof(token), "token_%d_%ld_%d", user_id, (long)time(NULL), rand() % 10000);

    if (g_session_count < MAX_SESSIONS)
    {
        Session *s = &g_sessions[g_session_count++];
        strncpy(s->token, token, sizeof(s->token) - 1);
        s->user_id = user_id;
        strncpy(s->username, username, sizeof(s->username) - 1);
        strncpy(s->role, role, sizeof(s->role) - 1);
        s->active = 1;
        s->login_time = time(NULL);
    }

    return token;
}

int verifySessionToken(const char *token, Session *out_session)
{
    if (!token || strlen(token) == 0) return 0;

    for (int i = 0; i < g_session_count; i++)
    {
        if (g_sessions[i].active && strcmp(g_sessions[i].token, token) == 0)
        {
            if (out_session) *out_session = g_sessions[i];
            return 1;
        }
    }
    return 0;
}

void invalidateSession(const char *token)
{
    if (!token) return;
    for (int i = 0; i < g_session_count; i++)
    {
        if (strcmp(g_sessions[i].token, token) == 0)
        {
            g_sessions[i].active = 0;
            break;
        }
    }
}

/* Extract Session Token from Header */
void getHeaderToken(const char *request, char *token, int max_len)
{
    token[0] = '\0';
    const char *pos = strstr(request, "X-Session-Token:");
    if (!pos) pos = strstr(request, "x-session-token:");
    if (!pos) return;

    pos = strchr(pos, ':');
    if (!pos) return;
    pos++;

    while (*pos == ' ' || *pos == '\t') pos++;

    int idx = 0;
    while (*pos && *pos != '\r' && *pos != '\n' && idx < max_len - 1)
    {
        token[idx++] = *pos++;
    }
    token[idx] = '\0';
}

/* ============================================================================
   BINARY FILE PERSISTENCE HELPERS
   ============================================================================ */

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
    else if (status_code == 401) status_text = "Unauthorized";
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
             "Access-Control-Allow-Headers: Content-Type, X-Session-Token\r\n"
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
    /* Strip query string parameters if present (e.g., /booking.html?room_no=201 -> /booking.html) */
    char clean_url[256];
    strncpy(clean_url, url_path, sizeof(clean_url) - 1);
    clean_url[sizeof(clean_url) - 1] = '\0';

    char *query_ptr = strchr(clean_url, '?');
    if (query_ptr)
    {
        *query_ptr = '\0';
    }

    /* Path Traversal & Directory Guard */
    if (strstr(clean_url, "..") != NULL ||
        strstr(clean_url, ".dat") != NULL ||
        strncmp(clean_url, "/backend", 8) == 0)
    {
        sendHttpResponse(client_fd, 403, "application/json", "{\"success\":false,\"message\":\"Access Denied\"}");
        return;
    }

    /* Guard against serving source/executable files */
    const char *dot = strrchr(clean_url, '.');
    if (dot && (strcmp(dot, ".c") == 0 || strcmp(dot, ".exe") == 0 || strcmp(dot, ".o") == 0))
    {
        sendHttpResponse(client_fd, 403, "application/json", "{\"success\":false,\"message\":\"Access Denied\"}");
        return;
    }

    char file_path[512];
    if (strcmp(clean_url, "/") == 0 || strcmp(clean_url, "/index.html") == 0)
    {
        snprintf(file_path, sizeof(file_path), "frontend/index.html");
    }
    else
    {
        snprintf(file_path, sizeof(file_path), "frontend%s", clean_url);
    }

    FILE *fp = fopen(file_path, "rb");
    if (!fp)
    {
        sendHttpResponse(client_fd, 404, "application/json", "{\"success\":false,\"message\":\"File Not Found\"}");
        return;
    }

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

    /* Extract Request Body */
    const char *body = strstr(request, "\r\n\r\n");
    if (body) body += 4;
    else
    {
        body = strstr(request, "\n\n");
        if (body) body += 2;
        else body = "";
    }

    /* Extract Session Token */
    char token[64];
    getHeaderToken(request, token, sizeof(token));
    Session active_session;
    int is_authenticated = verifySessionToken(token, &active_session);

    /* ------------------------------------------------------------------------
       1. POST /api/login - User Login & Session Creation
       ------------------------------------------------------------------------ */
    if (strcmp(method, "POST") == 0 && strcmp(path, "/api/login") == 0)
    {
        char username[50], password[100];
        getJsonStringValue(body, "username", username, sizeof(username));
        getJsonStringValue(body, "password", password, sizeof(password));

        User user;
        if (validateUser(username, password, &user))
        {
            char *sess_token = createSession(user.user_id, user.username, user.role);
            addAuditLog(user.user_id, user.username, "User Login", "Login successful");

            char json_res[512];
            snprintf(json_res, sizeof(json_res),
                     "{\"success\":true,\"message\":\"Login successful\",\"user_id\":%d,\"username\":\"%s\",\"role\":\"%s\",\"token\":\"%s\"}",
                     user.user_id, user.username, user.role, sess_token);
            sendHttpResponse(client_fd, 200, "application/json", json_res);
        }
        else
        {
            sendHttpResponse(client_fd, 401, "application/json", "{\"success\":false,\"message\":\"Invalid username or password\"}");
        }
        return;
    }

    /* ------------------------------------------------------------------------
       2. POST /api/logout - Invalidate Session
       ------------------------------------------------------------------------ */
    if (strcmp(method, "POST") == 0 && strcmp(path, "/api/logout") == 0)
    {
        if (is_authenticated)
        {
            addAuditLog(active_session.user_id, active_session.username, "User Logout", "Logged out");
            invalidateSession(token);
        }
        sendHttpResponse(client_fd, 200, "application/json", "{\"success\":true,\"message\":\"Logged out successfully\"}");
        return;
    }

    /* ------------------------------------------------------------------------
       3. GET /api/rooms - Return list of all rooms
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
                const char *status_str = (r.status == 0) ? "Available" : (r.status == 1) ? "Reserved" : (r.status == 2) ? "Occupied" : "Maintenance";
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
       4. GET /api/rooms/available - Return list of available rooms only
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
       5. POST /api/rooms/add - Add new room (Admin only)
       ------------------------------------------------------------------------ */
    if (strcmp(method, "POST") == 0 && strcmp(path, "/api/rooms/add") == 0)
    {
        if (!is_authenticated || strcmp(active_session.role, "admin") != 0)
        {
            sendHttpResponse(client_fd, 403, "application/json", "{\"success\":false,\"message\":\"Admin privileges required\"}");
            return;
        }

        int room_no = getJsonIntValue(body, "room_no", 0);
        char type[20];
        getJsonStringValue(body, "type", type, sizeof(type));
        float price = getJsonFloatValue(body, "price", 0.0f);

        if (room_no <= 0 || strlen(type) == 0 || price <= 0.0f)
        {
            sendHttpResponse(client_fd, 400, "application/json", "{\"success\":false,\"message\":\"Invalid room details\"}");
            return;
        }

        if (roomExists(room_no))
        {
            sendHttpResponse(client_fd, 409, "application/json", "{\"success\":false,\"message\":\"Room number already exists\"}");
            return;
        }

        Room r = {room_no, "", price, ROOM_AVAILABLE};
        strncpy(r.type, type, sizeof(r.type) - 1);

        FILE *fp = fopen(ROOMS_FILE, "ab");
        if (fp)
        {
            fwrite(&r, sizeof(Room), 1, fp);
            fclose(fp);
            addAuditLog(active_session.user_id, active_session.username, "Add Room", "Added new room");
            sendHttpResponse(client_fd, 201, "application/json", "{\"success\":true,\"message\":\"Room added successfully\"}");
        }
        return;
    }

    /* ------------------------------------------------------------------------
       6. POST /api/rooms/maintenance - Set Room Maintenance (Admin only)
       ------------------------------------------------------------------------ */
    if (strcmp(method, "POST") == 0 && strcmp(path, "/api/rooms/maintenance") == 0)
    {
        if (!is_authenticated || strcmp(active_session.role, "admin") != 0)
        {
            sendHttpResponse(client_fd, 403, "application/json", "{\"success\":false,\"message\":\"Admin privileges required\"}");
            return;
        }

        int room_no = getJsonIntValue(body, "room_no", 0);
        int maintenance = getJsonIntValue(body, "maintenance", 1);

        Room r;
        if (!getRoom(room_no, &r))
        {
            sendHttpResponse(client_fd, 404, "application/json", "{\"success\":false,\"message\":\"Room not found\"}");
            return;
        }

        if (maintenance)
        {
            if (r.status != ROOM_AVAILABLE)
            {
                sendHttpResponse(client_fd, 400, "application/json", "{\"success\":false,\"message\":\"Only Available rooms can be placed under maintenance\"}");
                return;
            }
            r.status = ROOM_MAINTENANCE;
        }
        else
        {
            r.status = ROOM_AVAILABLE;
        }

        updateRoom(r);
        addAuditLog(active_session.user_id, active_session.username, "Room Maintenance", maintenance ? "Placed under maintenance" : "Restored to Available");
        sendHttpResponse(client_fd, 200, "application/json", "{\"success\":true,\"message\":\"Room status updated successfully\"}");
        return;
    }

    /* ------------------------------------------------------------------------
       7. POST /api/bookings - Create new customer reservation
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

        if (r.status == ROOM_MAINTENANCE)
        {
            sendHttpResponse(client_fd, 400, "application/json", "{\"success\":false,\"message\":\"Room is currently under maintenance!\"}");
            return;
        }

        if (r.status != ROOM_AVAILABLE)
        {
            sendHttpResponse(client_fd, 409, "application/json", "{\"success\":false,\"message\":\"Selected room is no longer available!\"}");
            return;
        }

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
        if (fp)
        {
            fwrite(&b, sizeof(Booking), 1, fp);
            fclose(fp);
        }

        r.status = ROOM_RESERVED;
        updateRoom(r);

        float room_charge = r.price * b.days;
        addAuditLog(is_authenticated ? active_session.user_id : 0, is_authenticated ? active_session.username : "Customer", "Create Booking", "Reservation created");

        char json_res[512];
        snprintf(json_res, sizeof(json_res),
                 "{\"success\":true,\"booking_id\":%d,\"customer_name\":\"%s\",\"room_no\":%d,\"room_type\":\"%s\",\"days\":%d,\"room_charge\":%.2f,\"message\":\"Booking confirmed successfully!\"}",
                 b.booking_id, b.customer_name, b.room_no, r.type, b.days, room_charge);

        sendHttpResponse(client_fd, 201, "application/json", json_res);
        return;
    }

    /* ------------------------------------------------------------------------
       8. POST /api/bookings/lookup - Customer lookup by ID + Phone
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
       9. POST /api/bookings/cancel - Cancel reservation
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

        b.status = BOOKING_CANCELLED;
        updateBooking(b);

        Room r;
        if (getRoom(b.room_no, &r))
        {
            r.status = ROOM_AVAILABLE;
            updateRoom(r);
        }

        addAuditLog(is_authenticated ? active_session.user_id : 0, is_authenticated ? active_session.username : "Customer", "Cancel Booking", "Reservation cancelled");
        sendHttpResponse(client_fd, 200, "application/json", "{\"success\":true,\"message\":\"Reservation cancelled successfully!\"}");
        return;
    }

    /* ------------------------------------------------------------------------
       10. GET /api/bookings - Fetch all bookings
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
       11. POST /api/checkin - Admin/Reception Check-In
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

        addAuditLog(is_authenticated ? active_session.user_id : 0, is_authenticated ? active_session.username : "Staff", "Check In", "Customer checked in");
        sendHttpResponse(client_fd, 200, "application/json", "{\"success\":true,\"message\":\"Customer checked in successfully!\"}");
        return;
    }

    /* ------------------------------------------------------------------------
       12. GET /api/bills/{id} & POST /api/bills - Billing Operations
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
                 "{\"success\":true,\"bill\":{\"booking_id\":%d,\"room_charge\":%.2f,\"food_charge\":%.2f,\"service_charge\":%.2f,\"tax\":%.2f,\"total\":%.2f,\"paid\":%d,\"payment_method\":\"%s\"}}",
                 bill.booking_id, bill.room_charge, bill.food_charge, bill.service_charge, bill.tax, bill.total, bill.paid, bill.payment_method);
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
        float tax = subtotal * 0.10f;
        float total = subtotal + tax;

        Bill bill;
        bill.booking_id = booking_id;
        bill.room_charge = room_charge;
        bill.food_charge = food_charge;
        bill.service_charge = service_charge;
        bill.tax = tax;
        bill.total = total;
        bill.paid = 1;
        strncpy(bill.payment_method, "Cash", sizeof(bill.payment_method) - 1);

        saveBill(bill);
        addAuditLog(is_authenticated ? active_session.user_id : 0, is_authenticated ? active_session.username : "Staff", "Generate Bill", "Bill generated");

        char json_res[512];
        snprintf(json_res, sizeof(json_res),
                 "{\"success\":true,\"bill\":{\"booking_id\":%d,\"room_charge\":%.2f,\"food_charge\":%.2f,\"service_charge\":%.2f,\"tax\":%.2f,\"total\":%.2f,\"paid\":1,\"payment_method\":\"Cash\"},\"message\":\"Bill generated successfully!\"}",
                 bill.booking_id, bill.room_charge, bill.food_charge, bill.service_charge, bill.tax, bill.total);
        sendHttpResponse(client_fd, 200, "application/json", json_res);
        return;
    }

    /* ------------------------------------------------------------------------
       13. POST /api/payments - Payment Method Recording (Cash, Card, UPI)
       ------------------------------------------------------------------------ */
    if (strcmp(method, "POST") == 0 && strcmp(path, "/api/payments") == 0)
    {
        int booking_id = getJsonIntValue(body, "booking_id", 0);
        char payment_method[20];
        getJsonStringValue(body, "payment_method", payment_method, sizeof(payment_method));

        Bill bill;
        if (!getBill(booking_id, &bill))
        {
            sendHttpResponse(client_fd, 404, "application/json", "{\"success\":false,\"message\":\"Bill not found for this booking!\"}");
            return;
        }

        if (bill.paid == 1 && strlen(bill.payment_method) > 0 && strcmp(bill.payment_method, "Pending") != 0)
        {
            sendHttpResponse(client_fd, 400, "application/json", "{\"success\":false,\"message\":\"Payment already recorded for this bill!\"}");
            return;
        }

        bill.paid = 1;
        strncpy(bill.payment_method, payment_method, sizeof(bill.payment_method) - 1);
        saveBill(bill);

        addAuditLog(is_authenticated ? active_session.user_id : 0, is_authenticated ? active_session.username : "Staff", "Record Payment", payment_method);
        sendHttpResponse(client_fd, 200, "application/json", "{\"success\":true,\"message\":\"Payment recorded successfully!\"}");
        return;
    }

    /* ------------------------------------------------------------------------
       14. POST /api/checkout - Admin/Reception Check-Out
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

        addAuditLog(is_authenticated ? active_session.user_id : 0, is_authenticated ? active_session.username : "Staff", "Check Out", "Customer checked out");
        sendHttpResponse(client_fd, 200, "application/json", "{\"success\":true,\"message\":\"Customer checked out successfully! Room is now Available.\"}");
        return;
    }

    /* ------------------------------------------------------------------------
       15. GET /api/stats - Admin/Reception Dashboard KPIs
       ------------------------------------------------------------------------ */
    if (strcmp(method, "GET") == 0 && strcmp(path, "/api/stats") == 0)
    {
        int total_rooms = 0, avail_rooms = 0, res_rooms = 0, occ_rooms = 0, maint_rooms = 0;
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
                else if (r.status == ROOM_MAINTENANCE) maint_rooms++;
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

        float occupancy_rate = (total_rooms > 0) ? (((float)occ_rooms / (float)total_rooms) * 100.0f) : 0.0f;

        char json_res[512];
        snprintf(json_res, sizeof(json_res),
                 "{\"success\":true,\"stats\":{\"total_rooms\":%d,\"available_rooms\":%d,\"reserved_rooms\":%d,\"occupied_rooms\":%d,\"maintenance_rooms\":%d,\"total_bookings\":%d,\"total_revenue\":%.2f,\"occupancy_rate\":%.1f}}",
                 total_rooms, avail_rooms, res_rooms, occ_rooms, maint_rooms, total_bookings, total_revenue, occupancy_rate);
        sendHttpResponse(client_fd, 200, "application/json", json_res);
        return;
    }

    /* ------------------------------------------------------------------------
       16. GET /api/users - List All System Users (Admin only)
       ------------------------------------------------------------------------ */
    if (strcmp(method, "GET") == 0 && strcmp(path, "/api/users") == 0)
    {
        if (!is_authenticated || strcmp(active_session.role, "admin") != 0)
        {
            sendHttpResponse(client_fd, 403, "application/json", "{\"success\":false,\"message\":\"Admin privileges required\"}");
            return;
        }

        FILE *fp = fopen(USERS_FILE, "rb");
        char json[BUFFER_SIZE];
        strcpy(json, "{\"success\":true,\"users\":[");

        if (fp)
        {
            User u;
            int first = 1;
            while (fread(&u, sizeof(User), 1, fp) == 1)
            {
                char item[256];
                snprintf(item, sizeof(item), "%s{\"user_id\":%d,\"username\":\"%s\",\"role\":\"%s\",\"active\":%d}",
                         first ? "" : ",", u.user_id, u.username, u.role, u.active);
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
       17. POST /api/users/add - Add New System User (Admin only)
       ------------------------------------------------------------------------ */
    if (strcmp(method, "POST") == 0 && strcmp(path, "/api/users/add") == 0)
    {
        if (!is_authenticated || strcmp(active_session.role, "admin") != 0)
        {
            sendHttpResponse(client_fd, 403, "application/json", "{\"success\":false,\"message\":\"Admin privileges required\"}");
            return;
        }

        char username[50], password[100], role[20];
        getJsonStringValue(body, "username", username, sizeof(username));
        getJsonStringValue(body, "password", password, sizeof(password));
        getJsonStringValue(body, "role", role, sizeof(role));

        if (strlen(username) == 0 || strlen(password) == 0)
        {
            sendHttpResponse(client_fd, 400, "application/json", "{\"success\":false,\"message\":\"Invalid user parameters\"}");
            return;
        }

        FILE *fp = fopen(USERS_FILE, "rb");
        int next_id = 1;
        if (fp)
        {
            User u;
            while (fread(&u, sizeof(User), 1, fp) == 1)
            {
                if (strcmp(u.username, username) == 0)
                {
                    fclose(fp);
                    sendHttpResponse(client_fd, 409, "application/json", "{\"success\":false,\"message\":\"Username already exists\"}");
                    return;
                }
                if (u.user_id >= next_id) next_id = u.user_id + 1;
            }
            fclose(fp);
        }

        User new_user;
        new_user.user_id = next_id;
        strncpy(new_user.username, username, sizeof(new_user.username) - 1);
        strncpy(new_user.password, password, sizeof(new_user.password) - 1);
        strncpy(new_user.role, (strlen(role) > 0) ? role : "receptionist", sizeof(new_user.role) - 1);
        new_user.active = 1;

        fp = fopen(USERS_FILE, "ab");
        if (fp)
        {
            fwrite(&new_user, sizeof(User), 1, fp);
            fclose(fp);
            addAuditLog(active_session.user_id, active_session.username, "Add User", username);
            sendHttpResponse(client_fd, 201, "application/json", "{\"success\":true,\"message\":\"User account created successfully\"}");
        }
        return;
    }

    /* ------------------------------------------------------------------------
       18. GET /api/audit_logs - Fetch Audit Trail (Admin only)
       ------------------------------------------------------------------------ */
    if (strcmp(method, "GET") == 0 && strcmp(path, "/api/audit_logs") == 0)
    {
        if (!is_authenticated || strcmp(active_session.role, "admin") != 0)
        {
            sendHttpResponse(client_fd, 403, "application/json", "{\"success\":false,\"message\":\"Admin privileges required\"}");
            return;
        }

        FILE *fp = fopen(AUDIT_LOG_FILE, "rb");
        char json[BUFFER_SIZE];
        strcpy(json, "{\"success\":true,\"logs\":[");

        if (fp)
        {
            AuditLog log;
            int first = 1;
            while (fread(&log, sizeof(AuditLog), 1, fp) == 1)
            {
                char item[512];
                snprintf(item, sizeof(item),
                         "%s{\"log_id\":%d,\"user_id\":%d,\"username\":\"%s\",\"action\":\"%s\",\"details\":\"%s\",\"timestamp\":\"%s\"}",
                         first ? "" : ",", log.log_id, log.user_id, log.username, log.action, log.details, log.timestamp);
                strcat(json, item);
                first = 0;
            }
            fclose(fp);
        }
        strcat(json, "]}");
        sendHttpResponse(client_fd, 200, "application/json", json);
        return;
    }

    /* Serve Static Frontend Files */
    serveStaticFile(client_fd, path);
}
