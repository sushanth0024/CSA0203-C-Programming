# Hotel Room Reservation, Booking, Check-In & Billing Management System

A complete console-based **Hotel Management System** written in **C** using **binary file-based record processing** (`rooms.dat`, `bookings.dat`, `bills.dat`).

Developed for **CSA0203 - C Programming** college course.

---

## 🌟 Key Features

* **Room Management**: Add new rooms, view tabular room lists, search by room number, update room status with safety warnings.
* **Make Reservation**: Reserve available rooms, auto-generate unique Booking IDs (starting at `1001`), collect customer details with input validation (`days > 0`).
* **Check-In**: Verify reservation status, transition room from `RESERVED` to `OCCUPIED`.
* **Billing System**: Calculate room charges (`price × days`), add optional food & service charges, apply 10% tax, and store bill records in `bills.dat`.
* **Check-Out**: Verify checked-in status, display/generate bill, update booking status to `CHECKED-OUT`, and release room back to `AVAILABLE`.
* **Search & View Bookings**: View tabular booking records and comprehensive single-booking lookup.
* **Cancel Reservation**: Cancel reservations prior to check-in and release room back to `AVAILABLE`.
* **Binary File Persistence**: All room, booking, and billing data is saved to binary files and persists across application restarts.

---

## 📁 Repository Structure

```text
├── hotel_management.c   # Main C source code file
└── README.md            # Documentation
```

---

## 🚀 How to Compile and Run

### Prerequisites
* Any standard C compiler (such as **GCC**, **Clang**, or **MSVC**).

### Compilation (using GCC)
```bash
gcc -Wall -Wextra hotel_management.c -o hotel_management.exe
```

### Running the Application

**Windows (PowerShell / Command Prompt):**
```powershell
.\hotel_management.exe
```

**Linux / macOS:**
```bash
./hotel_management.exe
```

---

## 🔄 State Machine & Business Rules

```text
Available ──► Reserved ──► Occupied ──► Available
                 │
                 └──► Cancelled ──► Available
```

* **Room Status**: `0 = Available`, `1 = Reserved`, `2 = Occupied`
* **Booking Status**: `1 = Reserved`, `2 = Checked-In`, `3 = Checked-Out`, `4 = Cancelled`

---

## 📜 License
This project is open-source and intended for academic demonstration and learning.
