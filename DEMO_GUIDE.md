# Evaluator Demonstration Guide (5–10 Minute Presentation Sequence)

This guide provides a step-by-step sequence for demonstrating the **Web-Based Hotel Management System** during academic reviews or viva evaluations.

---

## ⏱️ Step-by-Step Demonstration Sequence

### Step 1: Launch C Web Server (30 Seconds)
1. Open PowerShell or Command Prompt in the repository folder:
   ```powershell
   cd "C:\Users\Sushanth\OneDrive\Documents\college\Hotel managment system"
   .\backend\hotel_server.exe
   ```
2. Point out:
   - C Socket HTTP Listener starting on `127.0.0.1:8080`.
   - Auto-creation/loading of binary databases (`rooms.dat`, `users.dat`).

---

### Step 2: Customer Homepage & Room Exploration (1 Minute)
1. Open **`http://127.0.0.1:8080`** in Google Chrome or Microsoft Edge.
2. Show:
   - Luxury dark theme layout, Google Fonts (`Cormorant Garamond` + `Plus Jakarta Sans`), glassmorphism search bar.
   - Dynamic room cards loaded directly from `rooms.dat` via `GET /api/rooms/available`.

---

### Step 3: Customer Reservation & Date Math (1.5 Minutes)
1. Click **BOOK NOW** on Room `201` (`Double`, ₹2,500/night).
2. Enter Customer Details: `Sushanth`, Phone `9876543210`, Email `sushanth@example.com`.
3. Select Check-In `20/08/2026` and Check-Out `23/08/2026`.
4. Point out the live cost estimation preview (3 nights = ₹7,500.00).
5. Click **CONFIRM BOOKING**.
6. Show the generated printable receipt voucher with Booking ID `1001` and status `RESERVED`.

---

### Step 4: Staff Authentication & Role-Based Access Control (1.5 Minutes)
1. Navigate to **Staff Portal** (`http://127.0.0.1:8080/admin.html`).
2. Log in as Receptionist: `reception` / `rec123`.
3. Point out:
   - Role badge displays `[RECEPTIONIST]`.
   - Administrative buttons (Add Room, Maintenance, User Management) are hidden.
4. Log out and log back in as Administrator: `admin` / `admin123`.
5. Point out:
   - Role badge displays `[ADMINISTRATOR]`.
   - Full administrative controls, room creation, maintenance toggle, and audit trail are unlocked.

---

### Step 5: Check-In, Billing & Check-Out Workflow (2 Minutes)
1. Click **🔑 Check-In** -> Enter Booking ID `1001`. Room 201 transitions from `Reserved` to `Occupied`.
2. Click **🧾 Generate Bill** -> Enter Booking ID `1001`, Food Charge ₹1,200, Service Charge ₹300. Show C backend tax calculation (10% Tax = ₹900, Total = ₹9,900.00).
3. Click **💳 Record Payment** -> Select `UPI`.
4. Click **🚪 Check-Out** -> Enter Booking ID `1001`. Room 201 releases back to `Available`.
5. Show Dashboard statistics updating (`Total Revenue: ₹19,800.00`).

---

### Step 6: Audit Trail & Data Persistence (1 Minute)
1. Scroll down to **Audit Trail & System Logs** on `admin.html`.
2. Show the real-time timestamped audit logs for every operation performed during the demo.
3. Stop the C server executable (`Ctrl + C`) and restart it.
4. Refresh the page to prove that all data survived process restart via binary `.dat` storage.
