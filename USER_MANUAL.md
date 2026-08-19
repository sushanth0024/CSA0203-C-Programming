# User Manual — Web-Based Hotel Management System

Welcome to the **Web-Based Hotel Room Reservation, Booking, Check-In, Check-Out, and Billing Management System**.

---

## 🌟 1. System Overview & Access

- **Web Portal URL**: `http://127.0.0.1:8080`
- **Staff Portal URL**: `http://127.0.0.1:8080/admin.html`
- **Default Demonstration Credentials**:
  - **Administrator**: Username `admin` | Password `admin123`
  - **Receptionist**: Username `reception` | Password `rec123`

---

## 🛎️ 2. Receptionist Guide

As a Receptionist, you have access to everyday hotel operations:

### 2.1 View Room Inventory & Availability
1. Navigate to **Rooms & Suites** (`http://127.0.0.1:8080/rooms.html`).
2. View available, reserved, occupied, and maintenance rooms with visual color-coded badges.

### 2.2 Create Customer Reservations
1. Navigate to **Bookings** or click **Book Now** on any available room card.
2. Enter the customer's **Full Name**, **Phone Number**, **Email**, **Check-In Date**, and **Check-Out Date**.
3. View the live estimated duration and room charge preview.
4. Click **Confirm Booking**. The system calculates the stay duration in C, generates a unique **Booking ID** (e.g., `1001`), updates the room status to `Reserved`, and redirects to a printable booking voucher.

### 2.3 Guest Check-In
1. Navigate to **Staff Portal** (`http://127.0.0.1:8080/admin.html`) and click **🔑 Check-In**.
2. Enter the customer's **Booking ID**.
3. Click **Confirm Check-In**. The system validates that the reservation exists, transitions the booking status to `Checked-In`, and sets the room status to `Occupied`.

### 2.4 Calculate & Generate Bill
1. On the Staff Portal, click **🧾 Generate Bill**.
2. Enter the **Booking ID**, any additional **Food Charge**, and **Service Charge**.
3. Click **Calculate Bill**. The C backend calculates:
   - `Room Charge = Nights * Daily Rate`
   - `Subtotal = Room Charge + Food + Service`
   - `Tax = 10% * Subtotal`
   - `Grand Total = Subtotal + Tax`
4. The generated bill is saved persistently to `bills.dat`.

### 2.5 Record Payment
1. On the Staff Portal, click **💳 Record Payment**.
2. Enter the **Booking ID** and select the payment method (**Cash**, **Credit/Debit Card**, or **UPI / NetBanking**).
3. Click **Record Payment**. The payment status updates to `Paid`.

### 2.6 Guest Check-Out
1. On the Staff Portal, click **🚪 Check-Out**.
2. Enter the **Booking ID**.
3. Click **Confirm Check-Out**. The system verifies that the customer is checked in and billed, updates the booking status to `Checked-Out`, and releases the room back to `Available`.

---

## 🔐 3. Administrator Guide

Administrators have full system privileges, including all Receptionist operations plus:

### 3.1 User Account Management
1. Log in as **`admin`**.
2. Click **👤 Add User**.
3. Enter a new username, password, and select the system role (**Administrator** or **Receptionist**).
4. Click **Create Account**.

### 3.2 Room Inventory & Maintenance Control
1. To add a new room: Click **➕ Add Room**, enter the room number, type (`Single`, `Double`, `Deluxe`, `Suite`), and daily rate (₹).
2. To place a room under maintenance: Click **🛠️ Room Maintenance**, enter the room number, and select **Place Under Maintenance**.
   - Rooms under maintenance cannot be reserved or checked in by staff or customers.
   - To restore the room: Select **Restore to Available**.

### 3.3 Audit Trail & System Logs
1. Scroll to the **Audit Trail & System Logs** section at the bottom of the Staff Portal.
2. View real-time timestamped audit records of all logins, logouts, booking creations, check-ins, check-outs, bills, payments, and maintenance updates.
