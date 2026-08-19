# Comprehensive Testing & Verification Report (Phase 5)

**Project Title**: Web-Based Hotel Room Reservation, Booking, Check-In, Check-Out, and Billing Management System  
**Backend**: Pure C (`backend/hotel_server.c`)  
**Storage Engine**: Binary `.dat` Record Engine (`rooms.dat`, `bookings.dat`, `bills.dat`, `users.dat`, `audit_logs.dat`)

---

## 1. Test Suite Summary Table

| Test ID | Test Category | Description | Input | Expected Result | Actual Result | Status |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **TC-01** | Authentication | Valid Admin Login | `admin` / `admin123` | `200 OK`, Session token returned, `role: admin` | Matches expected | **PASS** |
| **TC-02** | Authentication | Valid Receptionist Login | `reception` / `rec123` | `200 OK`, Session token returned, `role: receptionist` | Matches expected | **PASS** |
| **TC-03** | Authentication | Invalid Credentials | `admin` / `wrongpass` | `401 Unauthorized`, `Invalid username or password` | Matches expected | **PASS** |
| **TC-04** | RBAC Guard | Receptionist adding a room | Token from `reception` to `/api/rooms/add` | `403 Forbidden`, `Admin privileges required` | Matches expected | **PASS** |
| **TC-05** | RBAC Guard | Admin adding a room | Token from `admin` to `/api/rooms/add` | `201 Created`, Room saved to `rooms.dat` | Matches expected | **PASS** |
| **TC-06** | State Machine | Create Reservation | Room 201 (`Available`) | Booking `1001` created, Room status -> `Reserved` | Matches expected | **PASS** |
| **TC-07** | State Machine | Double-Booking Prevention | Reserve Room 201 again | `409 Conflict`, `Selected room is no longer available` | Matches expected | **PASS** |
| **TC-08** | State Machine | Maintenance Mode | Set Room 102 to Maintenance | Room status -> `Maintenance (3)` | Matches expected | **PASS** |
| **TC-09** | State Machine | Maintenance Protection | Reserve Room 102 | `400 Bad Request`, `Room is currently under maintenance` | Matches expected | **PASS** |
| **TC-10** | State Machine | Valid Check-In | Check-In Booking `1001` | Booking status -> `Checked-In`, Room status -> `Occupied` | Matches expected | **PASS** |
| **TC-11** | State Machine | Invalid Check-Out | Check-Out before billing | `400 Bad Request`, `Bill has not been generated` | Matches expected | **PASS** |
| **TC-12** | Billing Math | Bill Calculation | Room 201 (3 nights @ 2500) + Food 1200 + Service 300 | Subtotal = 9000, Tax (10%) = 900, Total = ₹9,900.00 | Matches expected | **PASS** |
| **TC-13** | Payment | Record Payment | Booking `1001`, Method `UPI` | `200 OK`, Payment status -> `Paid`, method `UPI` | Matches expected | **PASS** |
| **TC-14** | State Machine | Valid Check-Out | Check-Out Booking `1001` | Booking status -> `Checked-Out`, Room status -> `Available` | Matches expected | **PASS** |
| **TC-15** | Audit Logging | Audit Trail Generation | Perform login, booking, check-in, check-out | Logs written to `audit_logs.dat` with timestamps | Matches expected | **PASS** |
| **TC-16** | Security | Path Traversal Protection | Request `/../../secret.txt` or `/rooms.dat` | `403 Forbidden`, `Access Denied` | Matches expected | **PASS** |
| **TC-17** | Persistence | Server Restart Verification | Kill server process and restart `hotel_server.exe` | All room, booking, bill, user, and log records survive | Matches expected | **PASS** |

---

## 2. Test Execution Logs

### Billing Calculation Validation
```text
Room Charge:    3 nights × ₹2,500.00 = ₹7,500.00
Food Charge:    ₹1,200.00
Service Charge: ₹300.00
------------------------------------------------
Subtotal:       ₹9,000.00
Tax (10%):      ₹900.00
------------------------------------------------
Grand Total:    ₹9,900.00 (Calculated strictly in C backend)
```

### Persistence Verification Output
```json
{
  "success": true,
  "stats": {
    "total_rooms": 5,
    "available_rooms": 5,
    "reserved_rooms": 0,
    "occupied_rooms": 0,
    "total_bookings": 2,
    "total_revenue": 19800.00,
    "occupancy_rate": 0.0
  }
}
```
*Confirmed 100% data persistence across process restarts.*
