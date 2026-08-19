# Web-Based Hotel Management System

An end-to-end **Hotel Room Reservation, Booking, Check-In, Check-Out, and Billing Management System** built with a **Pure C HTTP Server (`backend/hotel_server.c`)**, binary file persistence engine (`rooms.dat`, `bookings.dat`, `bills.dat`, `users.dat`, `audit_logs.dat`), and a modern luxury **HTML5/CSS3/Vanilla JavaScript Frontend**.

---

## 🏛️ System Architecture Diagram

```text
                        CLIENT WEB BROWSER
             (HTML5 / CSS3 / Vanilla JS + Session Header)
                                │
                                │ HTTP GET / POST (Port 8080)
                                ▼
                     ┌─────────────────────┐
                     │    C HTTP SERVER    │
                     │  hotel_server.c     │
                     ├─────────────────────┤
                     │ Session Validator   │
                     │ Role-Based Router   │
                     │ Static File Server  │
                     │ Audit Log Engine    │
                     │ C Date & Tax Math   │
                     └──────────┬──────────┘
                                │
   ┌───────────┬───────────┬────┴──────┬───────────┬───────────┐
   ▼           ▼           ▼           ▼           ▼           ▼
rooms.dat  bookings.dat bills.dat  users.dat   guests.dat audit_logs.dat
```

---

## 🛠️ Data Flow

### Customer Reservation Flow
```text
Browser Form ──► POST /api/bookings ──► C Date Math (Nights) ──► Write bookings.dat ──► Update rooms.dat (Reserved) ──► JSON Voucher
```

### Guest Check-In & Billing Flow
```text
Check-In Modal ──► POST /api/checkin ──► Validate Status (Reserved) ──► Update rooms.dat (Occupied) ──► Audit Log
Generate Bill ──► POST /api/bills ──► Compute Subtotal + 10% Tax ──► Save bills.dat ──► Audit Log
Check-Out ──► POST /api/checkout ──► Verify Bill Exists ──► Release Room (Available) ──► Audit Log
```

---

## 📁 Repository Directory Structure

```text
Hotel Management System/
│
├── backend/
│   ├── hotel_server.c      # Pure C socket HTTP server & REST controller
│   ├── hotel_server.exe    # Compiled executable
│   ├── rooms.dat           # Room inventory binary database
│   ├── bookings.dat        # Booking records binary database
│   ├── bills.dat          # Billing & payment records binary database
│   ├── users.dat          # User account binary database
│   └── audit_logs.dat     # Audit trail binary database
│
├── frontend/
│   ├── index.html          # Customer homepage
│   ├── rooms.html          # Room catalog
│   ├── booking.html        # Customer booking form
│   ├── booking-confirmation.html # Printable booking voucher
│   ├── my-booking.html     # Customer booking lookup & cancellation
│   ├── admin.html          # Staff & Admin management portal
│   │
│   ├── css/
│   │   └── style.css       # Luxury dark theme stylesheet
│   │
│   └── js/
│       ├── api.js          # Fetch API wrapper
│       ├── main.js         # Homepage UI logic
│       ├── booking.js      # Booking submission & lookup
│       └── admin.js        # Staff portal & dashboard controllers
│
├── Makefile                # Cross-platform build script
├── API_DOCUMENTATION.md    # Complete REST API reference
├── USER_MANUAL.md          # Step-by-step user guide for Receptionists & Admins
├── TESTING_REPORT.md       # Full test suite & verification report
├── DEMO_GUIDE.md           # 5-10 minute presentation sequence
└── VIVA_QUESTIONS.md       # 30+ Viva Voce questions & answers
```

---

## 🚀 How to Compile & Run

### 1. Build using Makefile
- **Linux**:
  ```bash
  make
  ```
- **Windows (MinGW GCC)**:
  ```powershell
  make
  ```

### 2. Manual GCC Compilation
- **Windows**:
  ```powershell
  gcc -Wall -Wextra backend/hotel_server.c -o backend/hotel_server.exe -lws2_32
  ```

### 3. Execution
```powershell
.\backend\hotel_server.exe
```
Open **`http://127.0.0.1:8080`** in your browser.

---

## 🔑 Demonstration Credentials

| Role | Username | Password | Privileges |
| :--- | :--- | :--- | :--- |
| **Administrator** | `admin` | `admin123` | Full access, User creation, Room creation, Room maintenance, Audit logs |
| **Receptionist** | `reception` | `rec123` | Room catalog, Reservations, Guest check-in, Billing, Payments, Check-out |

---

## 🔐 Security Considerations & Limitations

- **Path Security**: Rejects path traversal attempts (`..`, `.dat`, `.c`, `.exe`, `/backend` access).
- **Session Tokens**: Generates 64-character in-memory session tokens; checks roles on protected API routes.
- **Academic Scope Note**: Demonstrations use plain-text password storage in binary files (`users.dat`) to adhere to standard C library constraints without third-party crypto dependencies.
