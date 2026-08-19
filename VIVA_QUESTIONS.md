# Comprehensive Viva Voce Questions & Answers Guide

This document contains **30+ detailed Viva Voce Questions & Answers** designed to prepare you for technical questions during your project defense.

---

## 💻 Section 1: C Programming & Systems Fundamentals

### Q1: Why did you build the backend in pure C without frameworks like Node.js or Python?
> **Answer**: Building the HTTP server in pure C demonstrates core low-level system concepts: network socket programming, binary buffer management, manual JSON parsing, structures, and direct file IO using system calls. It offers maximum execution efficiency, minimal memory footprint, and zero external runtime dependencies.

### Q2: How do you handle file storage in C without a database like MySQL or SQLite?
> **Answer**: Persistent records are stored as binary structures (`Room`, `Booking`, `Bill`, `User`, `AuditLog`) in `.dat` files (`rooms.dat`, `bookings.dat`, etc.). Standard C file functions (`fopen`, `fread`, `fwrite`, `fclose`) read and write binary records directly to disk. Record modifications use a temporary file rewrite pattern (`remove` and `rename`).

### Q3: What is the difference between text mode (`"w"`) and binary mode (`"wb"`) in `fopen()`?
> **Answer**: Text mode performs platform-specific line-ending translations (e.g., converting `\n` to `\r\n` on Windows). Binary mode (`"wb"`, `"rb"`) writes exact, un-translated byte representations of memory structures, preserving data integrity across different operating systems.

### Q4: How do you prevent buffer overflows in your C code?
> **Answer**: I avoid unsafe functions like `strcpy` and `sprintf`. Instead, I use bounds-checked functions like `strncpy(dest, src, max_len - 1)`, `snprintf()`, and explicit bounds checking before manipulating strings or buffers.

### Q5: How is date math (duration calculation) implemented in C?
> **Answer**: Standard C `struct tm` and `mktime()` convert date strings (`DD/MM/YYYY` or `YYYY-MM-DD`) into Unix epoch seconds (`time_t`). The difference in seconds is divided by `(24 * 3600)` to calculate the exact number of nights for a stay.

---

## 🌐 Section 2: Networking, Sockets & Web APIs

### Q6: How does your C program act as an HTTP web server?
> **Answer**: The server creates a TCP socket (`socket(AF_INET, SOCK_STREAM, 0)`), binds to IP `127.0.0.1` port `8080` (`bind()`), and listens for incoming connections (`listen()`). In a loop, it accepts client connections (`accept()`), reads the raw HTTP request stream (`recv()`), parses the HTTP method and URL path, routes the request to an API controller or static file handler, and writes the HTTP response (`send()`).

### Q7: What libraries are used for Windows socket support?
> **Answer**: On Windows, the WinSock2 library (`#include <winsock2.h>`) is initialized with `WSAStartup(MAKEWORD(2,2), &wsaData)` and linked using `-lws2_32`. On Linux, standard POSIX sockets (`<sys/socket.h>`, `<netinet/in.h>`) are used.

### Q8: How does your static file server host HTML, CSS, and JavaScript files?
> **Answer**: The server maps URL requests (e.g., `/css/style.css`) to local disk paths (`frontend/css/style.css`). It inspects the file extension to set the correct `Content-Type` header (`text/html`, `text/css`, `application/javascript`), reads the file content into memory, and sends it back to the web browser.

### Q9: How do you prevent path traversal security vulnerabilities (e.g., `/../../secret.txt`)?
> **Answer**: The server inspects requested static URLs for path traversal sequences (`..`), restricts requests outside `frontend/`, and explicitly blocks requests for internal `.dat` database files, `.c` source files, and `.exe` executables, returning `403 Forbidden` if violated.

### Q10: What is CORS (Cross-Origin Resource Sharing) and how is it supported?
> **Answer**: CORS allows web browsers to make requests to servers hosted on different origins. The C server includes CORS HTTP headers (`Access-Control-Allow-Origin: *`, `Access-Control-Allow-Methods: GET, POST, OPTIONS`) and handles HTTP `OPTIONS` pre-flight requests.

---

## 🏨 Section 3: Hotel Business Logic & State Machines

### Q11: Explain the Room State Machine in your system.
> **Answer**: A room moves through controlled state transitions:
> - `Available (0) -> Reserved (1)` upon booking.
> - `Reserved (1) -> Occupied (2)` upon guest check-in.
> - `Occupied (2) -> Available (0)` upon guest check-out.
> - `Available (0) <-> Maintenance (3)` for room maintenance.

### Q12: How do you prevent double-booking of a room?
> **Answer**: When a booking request arrives, the C server reads `rooms.dat` and checks `r.status`. If `r.status != ROOM_AVAILABLE`, the server rejects the request with HTTP `409 Conflict` (`"Selected room is no longer available!"`). This validation happens on the backend, ensuring two simultaneous requests cannot double-book a room.

### Q13: Can a guest check out before generating a bill?
> **Answer**: No. The check-out controller verifies that a corresponding bill record exists in `bills.dat`. If missing, the C server rejects the check-out with HTTP `400 Bad Request` (`"Bill has not been generated for this booking yet!"`).

### Q14: How is tax and total bill calculated?
> **Answer**: All billing math is computed on the C backend:
> - `Room Charge = Nights * Room Rate`
> - `Subtotal = Room Charge + Food Charge + Service Charge`
> - `Tax = Subtotal * 0.10` (10% Tax Rate)
> - `Grand Total = Subtotal + Tax`

### Q15: What payment methods are supported?
> **Answer**: Payments can be recorded as `Cash`, `Card`, or `UPI`. Recording a payment sets `bill.paid = 1` and updates `bills.dat`. Subsequent payment attempts for an already paid bill are rejected.

---

## 🔒 Section 4: Security, RBAC & Authentication

### Q16: How does user authentication work?
> **Answer**: The user submits credentials to `POST /api/login`. The server checks `users.dat`. Upon matching username and password for an active account, the C server generates a 64-character session token mapped to the user ID and role in memory.

### Q17: How is Role-Based Access Control (RBAC) enforced?
> **Answer**: Frontend JavaScript sends the session token in the HTTP header (`X-Session-Token`). Protected C API controllers check the token against the active session table. If a Receptionist attempts to access an Admin-only endpoint (like adding rooms or managing users), the C server returns HTTP `403 Forbidden`.

### Q18: What is the difference between Authentication and Authorization?
> **Answer**: Authentication verifies *who the user is* (username/password validation at login). Authorization determines *what the user is permitted to do* (checking whether an authenticated user has the `admin` role before granting access to sensitive routes).

### Q19: Where is audit logging performed?
> **Answer**: Every major administrative action (login, logout, booking, check-in, check-out, billing, payment, maintenance) invokes `addAuditLog()` in C. It records the user ID, username, action string, details, and exact timestamp to `audit_logs.dat`.

### Q20: How are passwords stored and what are the security limitations?
> **Answer**: In this academic implementation, demonstration passwords are stored directly in binary format within `users.dat` to maintain pure C standard library constraints. For production, password hashing (like bcrypt or Argon2) would be introduced.

---

## 📊 Section 5: Architecture & Data Persistence

### Q21: What happens to data when the server is turned off?
> **Answer**: All data remains persistent because records are written directly to binary files (`rooms.dat`, `bookings.dat`, `bills.dat`, `users.dat`, `audit_logs.dat`) on disk. When the server restarts, `initializeDatabases()` loads existing files without overwriting existing records.

### Q22: How do you handle room record updates in binary files?
> **Answer**: I use a temporary file rewrite pattern: open `rooms.dat` for reading and `temp_rooms.dat` for writing, write updated records, close both files, delete `rooms.dat`, and rename `temp_rooms.dat` to `rooms.dat`.

### Q23: How are booking IDs generated?
> **Answer**: `generateBookingID()` scans `bookings.dat` to find the highest existing ID (starting at 1000) and returns `max_id + 1`.

### Q24: What is the performance complexity of searching records in file-based persistence?
> **Answer**: Sequential file scanning has an $O(N)$ time complexity where $N$ is the number of records. For an academic scale (hundreds of rooms/bookings), disk I/O takes less than 1 millisecond.

### Q25: How does the client-side JavaScript communicate with C?
> **Answer**: Client-side JavaScript uses the browser `fetch()` API (`frontend/js/api.js`). It sends JSON payloads over HTTP POST/GET requests to `http://127.0.0.1:8080/api/...` and receives JSON responses.

---

## 🎨 Section 6: Frontend & UI Design

### Q26: What frontend technologies were used?
> **Answer**: Standard HTML5, CSS3, and Vanilla JavaScript without external frameworks like React, Vue, or Bootstrap.

### Q27: How is printable invoice functionality implemented?
> **Answer**: Printable invoices use CSS `@media print` rules in `frontend/css/style.css`. When `window.print()` is invoked, navigation bars, buttons, and admin background elements are hidden, rendering a clean black-and-white physical receipt.

### Q28: How does the UI adjust dynamically based on user role?
> **Answer**: Upon login, user role information is stored in browser `localStorage`. `checkAuthState()` toggles DOM elements marked with `.admin-only`, hiding admin controls from receptionists.

### Q29: What visual design choices were made for the luxury theme?
> **Answer**: A modern dark mode theme using Obsidian Charcoal (`#0b0f19`), Glassmorphic search cards (`backdrop-filter: blur(20px)`), Royal Gold accents (`#f59e0b`), and Google Fonts (`Cormorant Garamond` serif headers and `Plus Jakarta Sans` body text).

### Q30: How are occupancy rates calculated on the dashboard?
> **Answer**: The `/api/stats` controller counts occupied rooms and total rooms from `rooms.dat` and computes:
> $$\text{Occupancy Rate} = \left(\frac{\text{Occupied Rooms}}{\text{Total Rooms}}\right) \times 100$$
> The percentage is returned in the JSON stats payload and displayed on the dashboard KPI grid.
