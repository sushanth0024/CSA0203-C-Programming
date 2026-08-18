/*
================================================================================
  HOTEL ROOM RESERVATION, BOOKING, CHECK-IN, AND BILLING MANAGEMENT SYSTEM
================================================================================
  Language: C (Standard ANSI/ISO C)
  Storage:  Binary Files (rooms.dat, bookings.dat, bills.dat)
  Features: Room Management, Reservations, Check-In, Billing, Check-Out,
            Booking Search, Cancellation, Input Validation & Persistence.
================================================================================
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ============================================================================
   CONSTANTS AND FILE PATHS
   ============================================================================ */
#define ROOMS_FILE    "rooms.dat"
#define BOOKINGS_FILE "bookings.dat"
#define BILLS_FILE    "bills.dat"

/* Room Status Constants */
#define ROOM_AVAILABLE 0
#define ROOM_RESERVED  1
#define ROOM_OCCUPIED  2

/* Booking Status Constants */
#define BOOKING_RESERVED  1
#define BOOKING_CHECKED_IN 2
#define BOOKING_CHECKED_OUT 3
#define BOOKING_CANCELLED 4

/* ============================================================================
   DATA STRUCTURES
   ============================================================================ */

/* Room Structure */
typedef struct
{
    int room_no;
    char type[20];
    float price;
    int status; /* 0 = Available, 1 = Reserved, 2 = Occupied */
} Room;

/* Booking Structure */
typedef struct
{
    int booking_id;
    char customer_name[50];
    char phone[15];
    int room_no;
    int days;
    char check_in_date[15];
    char check_out_date[15];
    int status; /* 1 = Reserved, 2 = Checked-In, 3 = Checked-Out, 4 = Cancelled */
} Booking;

/* Bill Structure */
typedef struct
{
    int booking_id;
    float room_charge;
    float food_charge;
    float service_charge;
    float tax;
    float total;
} Bill;

/* ============================================================================
   FUNCTION DECLARATIONS (PROTOTYPES)
   ============================================================================ */
void clearScreen(void);
void pressEnter(void);
void clearInputBuffer(void);
void getStringInput(char *buffer, int max_len);
int getIntInput(const char *prompt);
float getFloatInput(const char *prompt);

void initializeRooms(void);

int roomExists(int room_no);
int getRoom(int room_no, Room *room);
int updateRoom(Room updated_room);
void getRoomStatusString(int status, char *output);

int bookingExists(int booking_id);
int getBooking(int booking_id, Booking *booking);
int updateBooking(Booking updated_booking);
int generateBookingID(void);
void getBookingStatusString(int status, char *output);

int getBill(int booking_id, Bill *bill);
int saveBill(Bill bill);

void addRoom(void);
void viewRooms(void);
void searchRoom(void);
void updateRoomStatusManual(void);
void roomManagementMenu(void);

void makeReservation(void);
void checkIn(void);
void billingModule(void);
void checkOut(void);
void searchBooking(void);
void viewBookings(void);
void cancelBooking(void);

void mainMenu(void);

/* ============================================================================
   MAIN FUNCTION
   ============================================================================ */
int main(void)
{
    /* Initialize default rooms if rooms.dat does not exist */
    initializeRooms();

    /* Launch the main system menu */
    mainMenu();

    return 0;
}

/* ============================================================================
   UTILITY AND UI HELPER FUNCTIONS
   ============================================================================ */

/* Clear terminal screen cross-platform */
void clearScreen(void)
{
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

/* Pause execution until user presses Enter */
void pressEnter(void)
{
    printf("\nPress Enter to continue...");
    getchar();
}

/* Flush standard input buffer to clear leftover characters/newlines */
void clearInputBuffer(void)
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF)
    {
        /* Read until newline or end of file */
    }
}

/* Safely read a string from input using fgets and strip trailing newline */
void getStringInput(char *buffer, int max_len)
{
    if (fgets(buffer, max_len, stdin) != NULL)
    {
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n')
        {
            buffer[len - 1] = '\0';
        }
    }
}

/* Read integer input with validation against non-numeric entries */
int getIntInput(const char *prompt)
{
    int val;
    char buffer[50];
    while (1)
    {
        printf("%s", prompt);
        if (fgets(buffer, sizeof(buffer), stdin) != NULL)
        {
            if (sscanf(buffer, "%d", &val) == 1)
            {
                return val;
            }
        }
        printf("Invalid input! Please enter a valid number.\n");
    }
}

/* Read floating-point input with validation */
float getFloatInput(const char *prompt)
{
    float val;
    char buffer[50];
    while (1)
    {
        printf("%s", prompt);
        if (fgets(buffer, sizeof(buffer), stdin) != NULL)
        {
            if (sscanf(buffer, "%f", &val) == 1)
            {
                return val;
            }
        }
        printf("Invalid input! Please enter a valid decimal number.\n");
    }
}

/* Convert room status integer code to descriptive string */
void getRoomStatusString(int status, char *output)
{
    switch (status)
    {
    case ROOM_AVAILABLE:
        strcpy(output, "Available");
        break;
    case ROOM_RESERVED:
        strcpy(output, "Reserved");
        break;
    case ROOM_OCCUPIED:
        strcpy(output, "Occupied");
        break;
    default:
        strcpy(output, "Unknown");
        break;
    }
}

/* Convert booking status integer code to descriptive string */
void getBookingStatusString(int status, char *output)
{
    switch (status)
    {
    case BOOKING_RESERVED:
        strcpy(output, "RESERVED");
        break;
    case BOOKING_CHECKED_IN:
        strcpy(output, "CHECKED-IN");
        break;
    case BOOKING_CHECKED_OUT:
        strcpy(output, "CHECKED-OUT");
        break;
    case BOOKING_CANCELLED:
        strcpy(output, "CANCELLED");
        break;
    default:
        strcpy(output, "UNKNOWN");
        break;
    }
}

/* ============================================================================
   FILE & DATA MANAGEMENT HELPERS
   ============================================================================ */

/* Initialize default sample rooms on first application run */
void initializeRooms(void)
{
    FILE *fp = fopen(ROOMS_FILE, "rb");
    if (fp != NULL)
    {
        /* File already exists, no need to overwrite */
        fclose(fp);
        return;
    }

    /* Seed default hotel rooms */
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
        printf("Error initializing room database file!\n");
        return;
    }

    fwrite(default_rooms, sizeof(Room), count, fp);
    fclose(fp);
}

/* Check if a room exists by room number */
int roomExists(int room_no)
{
    Room r;
    return getRoom(room_no, &r);
}

/* Fetch a room record by room number */
int getRoom(int room_no, Room *room)
{
    FILE *fp = fopen(ROOMS_FILE, "rb");
    if (!fp) return 0;

    Room temp;
    while (fread(&temp, sizeof(Room), 1, fp) == 1)
    {
        if (temp.room_no == room_no)
        {
            if (room != NULL)
            {
                *room = temp;
            }
            fclose(fp);
            return 1;
        }
    }

    fclose(fp);
    return 0;
}

/* Update a room record using temporary file pattern */
int updateRoom(Room updated_room)
{
    FILE *fp = fopen(ROOMS_FILE, "rb");
    if (!fp) return 0;

    FILE *temp_fp = fopen("temp_rooms.dat", "wb");
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
    rename("temp_rooms.dat", ROOMS_FILE);

    return updated;
}

/* Check if a booking exists by booking ID */
int bookingExists(int booking_id)
{
    Booking b;
    return getBooking(booking_id, &b);
}

/* Fetch a booking record by booking ID */
int getBooking(int booking_id, Booking *booking)
{
    FILE *fp = fopen(BOOKINGS_FILE, "rb");
    if (!fp) return 0;

    Booking temp;
    while (fread(&temp, sizeof(Booking), 1, fp) == 1)
    {
        if (temp.booking_id == booking_id)
        {
            if (booking != NULL)
            {
                *booking = temp;
            }
            fclose(fp);
            return 1;
        }
    }

    fclose(fp);
    return 0;
}

/* Update a booking record using temporary file pattern */
int updateBooking(Booking updated_booking)
{
    FILE *fp = fopen(BOOKINGS_FILE, "rb");
    if (!fp) return 0;

    FILE *temp_fp = fopen("temp_bookings.dat", "wb");
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
    rename("temp_bookings.dat", BOOKINGS_FILE);

    return updated;
}

/* Generate auto-incrementing unique Booking ID starting from 1001 */
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

/* Fetch a bill record by booking ID */
int getBill(int booking_id, Bill *bill)
{
    FILE *fp = fopen(BILLS_FILE, "rb");
    if (!fp) return 0;

    Bill temp;
    while (fread(&temp, sizeof(Bill), 1, fp) == 1)
    {
        if (temp.booking_id == booking_id)
        {
            if (bill != NULL)
            {
                *bill = temp;
            }
            fclose(fp);
            return 1;
        }
    }

    fclose(fp);
    return 0;
}

/* Save or update a bill record in bills.dat */
int saveBill(Bill new_bill)
{
    FILE *fp = fopen(BILLS_FILE, "rb");
    FILE *temp_fp = fopen("temp_bills.dat", "wb");
    int updated = 0;

    if (fp != NULL && temp_fp != NULL)
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
        rename("temp_bills.dat", BILLS_FILE);
    }
    else
    {
        if (fp != NULL) fclose(fp);
        if (temp_fp != NULL) fclose(temp_fp);
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
   1. ROOM MANAGEMENT MODULE
   ============================================================================ */

/* Add a new room to the database */
void addRoom(void)
{
    clearScreen();
    printf("========================================================\n");
    printf("                       ADD ROOM                         \n");
    printf("========================================================\n\n");

    int room_no = getIntInput("Enter Room Number: ");

    /* Input Validation: Prevent duplicate room numbers */
    if (roomExists(room_no))
    {
        printf("\n[ERROR] Room number %d already exists!\n", room_no);
        pressEnter();
        return;
    }

    Room r;
    r.room_no = room_no;

    printf("Enter Room Type (Single / Double / Deluxe / Suite): ");
    getStringInput(r.type, sizeof(r.type));

    /* Input Validation: Room price must be > 0 */
    while (1)
    {
        r.price = getFloatInput("Enter Price Per Day (Rupees): ");
        if (r.price > 0.0f)
        {
            break;
        }
        printf("[ERROR] Price must be greater than 0!\n");
    }

    r.status = ROOM_AVAILABLE;

    FILE *fp = fopen(ROOMS_FILE, "ab");
    if (!fp)
    {
        printf("\n[ERROR] Unable to open rooms database file!\n");
        pressEnter();
        return;
    }

    fwrite(&r, sizeof(Room), 1, fp);
    fclose(fp);

    printf("\n========================================================\n");
    printf("Room %d added successfully with status 'Available'!\n", r.room_no);
    printf("========================================================\n");
    pressEnter();
}

/* Display tabular view of all rooms */
void viewRooms(void)
{
    clearScreen();
    printf("========================================================\n");
    printf("                     VIEW ALL ROOMS                     \n");
    printf("========================================================\n\n");

    FILE *fp = fopen(ROOMS_FILE, "rb");
    if (!fp)
    {
        printf("No room records found!\n");
        pressEnter();
        return;
    }

    printf("--------------------------------------------------------\n");
    printf("%-10s %-15s %-15s %-12s\n", "Room No.", "Type", "Price/Day", "Status");
    printf("--------------------------------------------------------\n");

    Room r;
    char status_str[20];
    int count = 0;

    while (fread(&r, sizeof(Room), 1, fp) == 1)
    {
        getRoomStatusString(r.status, status_str);
        printf("%-10d %-15s %-15.2f %-12s\n", r.room_no, r.type, r.price, status_str);
        count++;
    }

    fclose(fp);

    printf("--------------------------------------------------------\n");
    printf("Total Rooms: %d\n", count);
    printf("--------------------------------------------------------\n");
    pressEnter();
}

/* Search for a room by room number */
void searchRoom(void)
{
    clearScreen();
    printf("========================================================\n");
    printf("                      SEARCH ROOM                       \n");
    printf("========================================================\n\n");

    int room_no = getIntInput("Enter Room Number to Search: ");

    Room r;
    if (getRoom(room_no, &r))
    {
        char status_str[20];
        getRoomStatusString(r.status, status_str);

        printf("\n--------------------------------------------------------\n");
        printf("Room Number : %d\n", r.room_no);
        printf("Room Type   : %s\n", r.type);
        printf("Price/Day   : Rupees %.2f\n", r.price);
        printf("Status      : %s\n", status_str);
        printf("--------------------------------------------------------\n");
    }
    else
    {
        printf("\n[ERROR] Room number %d not found!\n", room_no);
    }

    pressEnter();
}

/* Manual room status update with warning */
void updateRoomStatusManual(void)
{
    clearScreen();
    printf("========================================================\n");
    printf("                 UPDATE ROOM STATUS                     \n");
    printf("========================================================\n\n");

    int room_no = getIntInput("Enter Room Number to Update: ");

    Room r;
    if (!getRoom(room_no, &r))
    {
        printf("\n[ERROR] Room number %d not found!\n", room_no);
        pressEnter();
        return;
    }

    char status_str[20];
    getRoomStatusString(r.status, status_str);
    printf("\nCurrent Status for Room %d: %s\n", r.room_no, status_str);

    if (r.status == ROOM_RESERVED || r.status == ROOM_OCCUPIED)
    {
        printf("\n[WARNING] Room %d is currently assigned to an active booking!\n", r.room_no);
        printf("Manually updating status should only be done for maintenance/admin reasons.\n");
    }

    printf("\nSelect New Status:\n");
    printf("0. Available\n");
    printf("1. Reserved\n");
    printf("2. Occupied\n");

    int choice = getIntInput("Enter choice (0-2): ");
    if (choice < 0 || choice > 2)
    {
        printf("\n[ERROR] Invalid status choice!\n");
        pressEnter();
        return;
    }

    r.status = choice;
    if (updateRoom(r))
    {
        getRoomStatusString(r.status, status_str);
        printf("\n[SUCCESS] Room %d status updated to '%s'!\n", r.room_no, status_str);
    }
    else
    {
        printf("\n[ERROR] Failed to update room status!\n");
    }

    pressEnter();
}

/* Room Management Sub-Menu */
void roomManagementMenu(void)
{
    int choice;
    do
    {
        clearScreen();
        printf("========================================================\n");
        printf("                    ROOM MANAGEMENT                     \n");
        printf("========================================================\n");
        printf("1. Add Room\n");
        printf("2. View All Rooms\n");
        printf("3. Search Room\n");
        printf("4. Update Room Status (Manual)\n");
        printf("0. Back to Main Menu\n");
        printf("--------------------------------------------------------\n");

        choice = getIntInput("Enter your choice: ");

        switch (choice)
        {
        case 1:
            addRoom();
            break;
        case 2:
            viewRooms();
            break;
        case 3:
            searchRoom();
            break;
        case 4:
            updateRoomStatusManual();
            break;
        case 0:
            break;
        default:
            printf("\n[ERROR] Invalid choice! Please try again.\n");
            pressEnter();
            break;
        }
    } while (choice != 0);
}

/* ============================================================================
   2. MAKE RESERVATION MODULE
   ============================================================================ */
void makeReservation(void)
{
    clearScreen();
    printf("========================================================\n");
    printf("                    MAKE RESERVATION                    \n");
    printf("========================================================\n\n");

    /* Step 1: Display Available Rooms */
    FILE *fp = fopen(ROOMS_FILE, "rb");
    if (!fp)
    {
        printf("[ERROR] Rooms file not found!\n");
        pressEnter();
        return;
    }

    Room r;
    int available_count = 0;

    printf("Available Rooms:\n");
    printf("--------------------------------------------------------\n");
    printf("%-10s %-15s %-15s\n", "Room No.", "Type", "Price/Day");
    printf("--------------------------------------------------------\n");

    while (fread(&r, sizeof(Room), 1, fp) == 1)
    {
        if (r.status == ROOM_AVAILABLE)
        {
            printf("%-10d %-15s %-15.2f\n", r.room_no, r.type, r.price);
            available_count++;
        }
    }
    fclose(fp);

    if (available_count == 0)
    {
        printf("No rooms are currently available for reservation!\n");
        pressEnter();
        return;
    }
    printf("--------------------------------------------------------\n\n");

    /* Step 2: Select Room & Check Availability */
    int room_no = getIntInput("Select Room Number to Reserve: ");

    Room selected_room;
    if (!getRoom(room_no, &selected_room))
    {
        printf("\n[ERROR] Room number %d not found!\n", room_no);
        pressEnter();
        return;
    }

    if (selected_room.status != ROOM_AVAILABLE)
    {
        char status_str[20];
        getRoomStatusString(selected_room.status, status_str);
        printf("\n[ERROR] Room %d is not available! Current Status: %s\n", room_no, status_str);
        pressEnter();
        return;
    }

    /* Step 3: Enter Customer Details with Validation */
    Booking b;
    b.booking_id = generateBookingID();
    b.room_no = room_no;

    printf("Enter Customer Name: ");
    getStringInput(b.customer_name, sizeof(b.customer_name));
    while (strlen(b.customer_name) == 0)
    {
        printf("[ERROR] Customer name cannot be empty! Re-enter: ");
        getStringInput(b.customer_name, sizeof(b.customer_name));
    }

    printf("Enter Phone Number: ");
    getStringInput(b.phone, sizeof(b.phone));
    while (strlen(b.phone) == 0)
    {
        printf("[ERROR] Phone number cannot be empty! Re-enter: ");
        getStringInput(b.phone, sizeof(b.phone));
    }

    /* Validation: Days > 0 */
    while (1)
    {
        b.days = getIntInput("Enter Number of Days: ");
        if (b.days > 0)
        {
            break;
        }
        printf("[ERROR] Invalid number of days! Please enter a value greater than 0.\n");
    }

    printf("Enter Check-In Date (DD/MM/YYYY): ");
    getStringInput(b.check_in_date, sizeof(b.check_in_date));
    while (strlen(b.check_in_date) == 0)
    {
        printf("[ERROR] Check-In date cannot be empty! Re-enter: ");
        getStringInput(b.check_in_date, sizeof(b.check_in_date));
    }

    strcpy(b.check_out_date, "N/A");
    b.status = BOOKING_RESERVED;

    /* Step 4: Save Booking Record to File */
    FILE *booking_fp = fopen(BOOKINGS_FILE, "ab");
    if (!booking_fp)
    {
        printf("\n[ERROR] Unable to open bookings file!\n");
        pressEnter();
        return;
    }

    fwrite(&b, sizeof(Booking), 1, booking_fp);
    fclose(booking_fp);

    /* Step 5: Automatic Room Status Transition (Available -> Reserved) */
    selected_room.status = ROOM_RESERVED;
    updateRoom(selected_room);

    /* Step 6: Confirmation Message */
    printf("\n========================================================\n");
    printf("                RESERVATION SUCCESSFUL                  \n");
    printf("========================================================\n");
    printf("Booking ID     : %d\n", b.booking_id);
    printf("Customer Name  : %s\n", b.customer_name);
    printf("Phone Number   : %s\n", b.phone);
    printf("Room Number    : %d (%s)\n", b.room_no, selected_room.type);
    printf("Price / Day    : Rupees %.2f\n", selected_room.price);
    printf("Number of Days : %d\n", b.days);
    printf("Check-In Date  : %s\n", b.check_in_date);
    printf("Status         : RESERVED\n");
    printf("========================================================\n");

    pressEnter();
}

/* ============================================================================
   3. CHECK-IN MODULE
   ============================================================================ */
void checkIn(void)
{
    clearScreen();
    printf("========================================================\n");
    printf("                        CHECK-IN                        \n");
    printf("========================================================\n\n");

    int booking_id = getIntInput("Enter Booking ID for Check-In: ");

    Booking b;
    if (!getBooking(booking_id, &b))
    {
        printf("\n[ERROR] Booking ID %d not found!\n", booking_id);
        pressEnter();
        return;
    }

    /* Validation: Only allow check-in if status is Reserved */
    if (b.status == BOOKING_CHECKED_IN)
    {
        printf("\n[ERROR] Booking ID %d has already been checked in!\n", booking_id);
        pressEnter();
        return;
    }
    else if (b.status == BOOKING_CHECKED_OUT)
    {
        printf("\n[ERROR] Booking ID %d has already checked out!\n", booking_id);
        pressEnter();
        return;
    }
    else if (b.status == BOOKING_CANCELLED)
    {
        printf("\n[ERROR] Booking ID %d was cancelled and cannot be checked in!\n", booking_id);
        pressEnter();
        return;
    }

    /* Transition Booking Status: Reserved -> Checked-In */
    b.status = BOOKING_CHECKED_IN;
    updateBooking(b);

    /* Transition Room Status: Reserved -> Occupied */
    Room r;
    if (getRoom(b.room_no, &r))
    {
        r.status = ROOM_OCCUPIED;
        updateRoom(r);
    }

    printf("\n========================================================\n");
    printf("                  CHECK-IN SUCCESSFUL                   \n");
    printf("========================================================\n");
    printf("Booking ID     : %d\n", b.booking_id);
    printf("Customer Name  : %s\n", b.customer_name);
    printf("Room Number    : %d\n", b.room_no);
    printf("Check-In Date  : %s\n", b.check_in_date);
    printf("Booking Status : CHECKED-IN\n");
    printf("Room Status    : OCCUPIED\n");
    printf("========================================================\n");

    pressEnter();
}

/* Helper to prompt, calculate and save/display bill */
void generateOrDisplayBill(int booking_id)
{
    Booking b;
    if (!getBooking(booking_id, &b))
    {
        printf("\n[ERROR] Booking ID %d not found!\n", booking_id);
        return;
    }

    Room r;
    if (!getRoom(b.room_no, &r))
    {
        printf("\n[ERROR] Room data for Room %d not found!\n", b.room_no);
        return;
    }

    Bill bill;
    int bill_found = getBill(booking_id, &bill);

    if (bill_found)
    {
        printf("\n========================================================\n");
        printf("                     HOTEL BILL                         \n");
        printf("========================================================\n");
        printf("Booking ID      : %d\n", b.booking_id);
        printf("Customer        : %s\n", b.customer_name);
        printf("Room Number     : %d (%s)\n", b.room_no, r.type);
        printf("Number of Days  : %d\n\n", b.days);
        printf("Room Charge     : Rupees %.2f\n", bill.room_charge);
        printf("Food Charge     : Rupees %.2f\n", bill.food_charge);
        printf("Service Charge  : Rupees %.2f\n", bill.service_charge);
        printf("Tax (10%%)       : Rupees %.2f\n", bill.tax);
        printf("--------------------------------------------------------\n");
        printf("TOTAL           : Rupees %.2f\n", bill.total);
        printf("========================================================\n");
        printf("[Status: Bill already generated for this booking]\n");

        char choice_str[10];
        printf("\nDo you want to re-calculate / update this bill? (y/n): ");
        getStringInput(choice_str, sizeof(choice_str));
        if (choice_str[0] != 'y' && choice_str[0] != 'Y')
        {
            return;
        }
    }

    printf("\n--- Generating Bill ---\n");
    float room_charge = r.price * b.days;
    printf("Room Charge (%d days @ Rupees %.2f/day): Rupees %.2f\n", b.days, r.price, room_charge);

    float food_charge;
    while (1)
    {
        food_charge = getFloatInput("Enter Food Charge (Rupees, 0 if none): ");
        if (food_charge >= 0.0f) break;
        printf("[ERROR] Food charge cannot be negative!\n");
    }

    float service_charge;
    while (1)
    {
        service_charge = getFloatInput("Enter Service Charge (Rupees, 0 if none): ");
        if (service_charge >= 0.0f) break;
        printf("[ERROR] Service charge cannot be negative!\n");
    }

    float subtotal = room_charge + food_charge + service_charge;
    float tax = subtotal * 0.10f; /* 10% tax */
    float total = subtotal + tax;

    bill.booking_id = b.booking_id;
    bill.room_charge = room_charge;
    bill.food_charge = food_charge;
    bill.service_charge = service_charge;
    bill.tax = tax;
    bill.total = total;

    saveBill(bill);

    printf("\n========================================================\n");
    printf("                     HOTEL BILL                         \n");
    printf("========================================================\n");
    printf("Booking ID      : %d\n", b.booking_id);
    printf("Customer        : %s\n", b.customer_name);
    printf("Room Number     : %d (%s)\n", b.room_no, r.type);
    printf("Number of Days  : %d\n\n", b.days);
    printf("Room Charge     : Rupees %.2f\n", bill.room_charge);
    printf("Food Charge     : Rupees %.2f\n", bill.food_charge);
    printf("Service Charge  : Rupees %.2f\n", bill.service_charge);
    printf("Tax (10%%)       : Rupees %.2f\n", bill.tax);
    printf("--------------------------------------------------------\n");
    printf("TOTAL           : Rupees %.2f\n", bill.total);
    printf("========================================================\n");
}

/* ============================================================================
   4. BILLING MODULE
   ============================================================================ */
void billingModule(void)
{
    clearScreen();
    printf("========================================================\n");
    printf("                        BILLING                         \n");
    printf("========================================================\n\n");

    int booking_id = getIntInput("Enter Booking ID for Billing: ");

    Booking b;
    if (!getBooking(booking_id, &b))
    {
        printf("\n[ERROR] Booking ID %d not found!\n", booking_id);
        pressEnter();
        return;
    }

    generateOrDisplayBill(booking_id);
    pressEnter();
}

/* ============================================================================
   5. CHECK-OUT MODULE
   ============================================================================ */
void checkOut(void)
{
    clearScreen();
    printf("========================================================\n");
    printf("                       CHECK-OUT                        \n");
    printf("========================================================\n\n");

    int booking_id = getIntInput("Enter Booking ID for Check-Out: ");

    Booking b;
    if (!getBooking(booking_id, &b))
    {
        printf("\n[ERROR] Booking ID %d not found!\n", booking_id);
        pressEnter();
        return;
    }

    if (b.status == BOOKING_RESERVED)
    {
        printf("\n[ERROR] Customer has not checked in yet! Check-in required before check-out.\n");
        pressEnter();
        return;
    }
    else if (b.status == BOOKING_CHECKED_OUT)
    {
        printf("\n[ERROR] Booking ID %d has already checked out!\n", booking_id);
        pressEnter();
        return;
    }
    else if (b.status == BOOKING_CANCELLED)
    {
        printf("\n[ERROR] Booking ID %d was cancelled!\n", booking_id);
        pressEnter();
        return;
    }

    /* Ensure Bill has been generated before checkout */
    Bill bill;
    if (!getBill(booking_id, &bill))
    {
        printf("\n[NOTICE] No bill found for Booking ID %d. Generating bill now...\n", booking_id);
        generateOrDisplayBill(booking_id);
    }
    else
    {
        generateOrDisplayBill(booking_id);
    }

    printf("\nEnter Check-Out Date (DD/MM/YYYY): ");
    getStringInput(b.check_out_date, sizeof(b.check_out_date));
    while (strlen(b.check_out_date) == 0)
    {
        printf("[ERROR] Check-Out date cannot be empty! Re-enter: ");
        getStringInput(b.check_out_date, sizeof(b.check_out_date));
    }

    /* Transition Booking Status: Checked-In -> Checked-Out */
    b.status = BOOKING_CHECKED_OUT;
    updateBooking(b);

    /* Transition Room Status: Occupied -> Available */
    Room r;
    if (getRoom(b.room_no, &r))
    {
        r.status = ROOM_AVAILABLE;
        updateRoom(r);
    }

    printf("\n========================================================\n");
    printf("                 CHECK-OUT SUCCESSFUL                   \n");
    printf("========================================================\n");
    printf("Customer       : %s\n", b.customer_name);
    printf("Room Number    : %d\n", b.room_no);
    printf("Booking ID     : %d\n", b.booking_id);
    printf("Check-Out Date : %s\n", b.check_out_date);
    printf("Booking Status : CHECKED-OUT\n");
    printf("Room Status    : AVAILABLE\n");
    printf("========================================================\n");

    pressEnter();
}

/* ============================================================================
   6. SEARCH BOOKING MODULE
   ============================================================================ */
void searchBooking(void)
{
    clearScreen();
    printf("========================================================\n");
    printf("                     SEARCH BOOKING                     \n");
    printf("========================================================\n\n");

    int booking_id = getIntInput("Enter Booking ID to Search: ");

    Booking b;
    if (getBooking(booking_id, &b))
    {
        char status_str[20];
        getBookingStatusString(b.status, status_str);

        Room r;
        getRoom(b.room_no, &r);

        printf("\n========================================================\n");
        printf("                    BOOKING DETAILS                     \n");
        printf("========================================================\n");
        printf("Booking ID     : %d\n", b.booking_id);
        printf("Customer Name  : %s\n", b.customer_name);
        printf("Phone Number   : %s\n", b.phone);
        printf("Room Number    : %d (%s)\n", b.room_no, r.type);
        printf("Number of Days : %d\n", b.days);
        printf("Check-In Date  : %s\n", b.check_in_date);
        printf("Check-Out Date : %s\n", b.check_out_date);
        printf("Booking Status : %s\n", status_str);

        Bill bill;
        if (getBill(b.booking_id, &bill))
        {
            printf("Total Billed   : Rupees %.2f\n", bill.total);
        }
        else
        {
            printf("Total Billed   : Not Generated Yet\n");
        }
        printf("========================================================\n");
    }
    else
    {
        printf("\n[ERROR] Booking ID %d not found!\n", booking_id);
    }

    pressEnter();
}

/* ============================================================================
   8. VIEW ALL BOOKINGS MODULE
   ============================================================================ */
void viewBookings(void)
{
    clearScreen();
    printf("========================================================\n");
    printf("                   VIEW ALL BOOKINGS                    \n");
    printf("========================================================\n\n");

    FILE *fp = fopen(BOOKINGS_FILE, "rb");
    if (!fp)
    {
        printf("No booking records found!\n");
        pressEnter();
        return;
    }

    printf("----------------------------------------------------------------------------------------\n");
    printf("%-8s %-18s %-12s %-6s %-6s %-12s %-12s\n",
           "ID", "Customer", "Phone", "Room", "Days", "Check-In", "Status");
    printf("----------------------------------------------------------------------------------------\n");

    Booking b;
    char status_str[20];
    int count = 0;

    while (fread(&b, sizeof(Booking), 1, fp) == 1)
    {
        getBookingStatusString(b.status, status_str);
        printf("%-8d %-18.18s %-12.12s %-6d %-6d %-12.12s %-12s\n",
               b.booking_id, b.customer_name, b.phone, b.room_no, b.days, b.check_in_date, status_str);
        count++;
    }

    fclose(fp);

    printf("----------------------------------------------------------------------------------------\n");
    printf("Total Bookings: %d\n", count);
    printf("----------------------------------------------------------------------------------------\n");

    pressEnter();
}

/* ============================================================================
   9. CANCEL RESERVATION MODULE
   ============================================================================ */
void cancelBooking(void)
{
    clearScreen();
    printf("========================================================\n");
    printf("                   CANCEL RESERVATION                   \n");
    printf("========================================================\n\n");

    int booking_id = getIntInput("Enter Booking ID to Cancel: ");

    Booking b;
    if (!getBooking(booking_id, &b))
    {
        printf("\n[ERROR] Booking ID %d not found!\n", booking_id);
        pressEnter();
        return;
    }

    if (b.status == BOOKING_CHECKED_IN)
    {
        printf("\n[ERROR] Cannot cancel booking! Customer is currently checked in.\n");
        pressEnter();
        return;
    }
    else if (b.status == BOOKING_CHECKED_OUT)
    {
        printf("\n[ERROR] Cannot cancel booking! Customer has already checked out.\n");
        pressEnter();
        return;
    }
    else if (b.status == BOOKING_CANCELLED)
    {
        printf("\n[ERROR] Booking ID %d is already cancelled!\n", booking_id);
        pressEnter();
        return;
    }

    printf("\nBooking Found:\n");
    printf("Booking ID    : %d\n", b.booking_id);
    printf("Customer Name : %s\n", b.customer_name);
    printf("Room Number   : %d\n", b.room_no);

    char confirm_str[10];
    printf("\nAre you sure you want to cancel this reservation? (y/n): ");
    getStringInput(confirm_str, sizeof(confirm_str));

    if (confirm_str[0] == 'y' || confirm_str[0] == 'Y')
    {
        /* Transition Booking Status: Reserved -> Cancelled */
        b.status = BOOKING_CANCELLED;
        updateBooking(b);

        /* Transition Room Status: Reserved -> Available */
        Room r;
        if (getRoom(b.room_no, &r))
        {
            r.status = ROOM_AVAILABLE;
            updateRoom(r);
        }

        printf("\n[SUCCESS] Reservation ID %d has been cancelled successfully!\n", b.booking_id);
    }
    else
    {
        printf("\nCancellation aborted.\n");
    }

    pressEnter();
}

/* ============================================================================
   MAIN MENU DRIVER
   ============================================================================ */
void mainMenu(void)
{
    int choice;
    do
    {
        clearScreen();
        printf("========================================================\n");
        printf("              HOTEL MANAGEMENT SYSTEM                  \n");
        printf("========================================================\n\n");
        printf("1. Room Management\n");
        printf("2. Make Reservation\n");
        printf("3. Check-In\n");
        printf("4. Billing\n");
        printf("5. Check-Out\n");
        printf("6. Search Booking\n");
        printf("7. View All Rooms\n");
        printf("8. View All Bookings\n");
        printf("9. Cancel Reservation\n");
        printf("0. Exit\n\n");
        printf("--------------------------------------------------------\n");

        choice = getIntInput("Enter your choice: ");

        switch (choice)
        {
        case 1:
            roomManagementMenu();
            break;
        case 2:
            makeReservation();
            break;
        case 3:
            checkIn();
            break;
        case 4:
            billingModule();
            break;
        case 5:
            checkOut();
            break;
        case 6:
            searchBooking();
            break;
        case 7:
            viewRooms();
            break;
        case 8:
            viewBookings();
            break;
        case 9:
            cancelBooking();
            break;
        case 0:
            printf("\nThank you for using Hotel Management System. Goodbye!\n\n");
            break;
        default:
            printf("\n[ERROR] Invalid choice! Please select a valid option (0-9).\n");
            pressEnter();
            break;
        }
    } while (choice != 0);
}
