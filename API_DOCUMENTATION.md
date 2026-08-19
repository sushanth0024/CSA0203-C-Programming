# REST API Documentation — Web-Based Hotel Management System (Phase 4)

**Base URL**: `http://127.0.0.1:8080/api`  
**Authentication Header**: `X-Session-Token: <token>`

---

## 🔑 1. Authentication & Session APIs

### `POST /api/login`
- **Auth Required**: No
- **Request Body**:
  ```json
  {
    "username": "admin",
    "password": "admin123"
  }
  ```
- **Response** (`200 OK`):
  ```json
  {
    "success": true,
    "message": "Login successful",
    "user_id": 1,
    "username": "admin",
    "role": "admin",
    "token": "token_1_1787046000_1234"
  }
  ```

### `POST /api/logout`
- **Auth Required**: Yes
- **Response** (`200 OK`):
  ```json
  {
    "success": true,
    "message": "Logged out successfully"
  }
  ```

---

## 🛏️ 2. Room Management APIs

### `GET /api/rooms`
- **Auth Required**: No
- **Response** (`200 OK`):
  ```json
  {
    "success": true,
    "rooms": [
      { "room_no": 101, "type": "Single", "price": 1500.00, "status": 0, "status_str": "Available" },
      { "room_no": 201, "type": "Double", "price": 2500.00, "status": 1, "status_str": "Reserved" }
    ]
  }
  ```

### `GET /api/rooms/available`
- **Auth Required**: No
- **Response** (`200 OK`): Filters only rooms where `status == 0`.

### `POST /api/rooms/add`
- **Auth Required**: Yes (`admin` only)
- **Request Body**: `{"room_no": 401, "type": "Suite", "price": 5000.00}`
- **Response** (`201 Created`): `{"success": true, "message": "Room added successfully"}`

### `POST /api/rooms/maintenance`
- **Auth Required**: Yes (`admin` only)
- **Request Body**: `{"room_no": 101, "maintenance": 1}`
- **Response** (`200 OK`): `{"success": true, "message": "Room status updated successfully"}`

---

## 📅 3. Booking & Check-In / Check-Out APIs

### `POST /api/bookings`
- **Auth Required**: No
- **Request Body**:
  ```json
  {
    "customer_name": "Rahul Sharma",
    "phone": "9876500001",
    "email": "rahul@example.com",
    "room_no": 202,
    "check_in_date": "25/08/2026",
    "check_out_date": "28/08/2026",
    "guests": 2
  }
  ```
- **Response** (`201 Created`):
  ```json
  {
    "success": true,
    "booking_id": 1002,
    "customer_name": "Rahul Sharma",
    "room_no": 202,
    "room_type": "Double",
    "days": 3,
    "room_charge": 7500.00,
    "message": "Booking confirmed successfully!"
  }
  ```

### `POST /api/bookings/lookup`
- **Auth Required**: No
- **Request Body**: `{"booking_id": 1002, "phone": "9876500001"}`
- **Response** (`200 OK`): Returns booking details & status string.

### `POST /api/checkin`
- **Auth Required**: Yes (`admin` or `receptionist`)
- **Request Body**: `{"booking_id": 1002}`
- **Response** (`200 OK`): `{"success": true, "message": "Customer checked in successfully!"}`

### `POST /api/bills`
- **Auth Required**: Yes (`admin` or `receptionist`)
- **Request Body**: `{"booking_id": 1002, "food_charge": 1000, "service_charge": 500}`
- **Response** (`200 OK`):
  ```json
  {
    "success": true,
    "bill": {
      "booking_id": 1002,
      "room_charge": 7500.00,
      "food_charge": 1000.00,
      "service_charge": 500.00,
      "tax": 900.00,
      "total": 9900.00,
      "paid": 1,
      "payment_method": "Cash"
    }
  }
  ```

### `POST /api/payments`
- **Auth Required**: Yes (`admin` or `receptionist`)
- **Request Body**: `{"booking_id": 1002, "payment_method": "UPI"}`
- **Response** (`200 OK`): `{"success": true, "message": "Payment recorded successfully!"}`

### `POST /api/checkout`
- **Auth Required**: Yes (`admin` or `receptionist`)
- **Request Body**: `{"booking_id": 1002}`
- **Response** (`200 OK`): `{"success": true, "message": "Customer checked out successfully!"}`

---

## 👥 4. User & Audit Trail APIs (Admin Only)

### `GET /api/users`
- **Auth Required**: Yes (`admin` only)
- **Response** (`200 OK`): Returns list of system user accounts.

### `POST /api/users/add`
- **Auth Required**: Yes (`admin` only)
- **Request Body**: `{"username": "rec_staff", "password": "pass123", "role": "receptionist"}`
- **Response** (`201 Created`): `{"success": true, "message": "User account created successfully"}`

### `GET /api/audit_logs`
- **Auth Required**: Yes (`admin` only)
- **Response** (`200 OK`): Returns complete audit log sequence.
