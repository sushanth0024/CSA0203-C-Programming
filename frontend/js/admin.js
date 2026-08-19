/* ============================================================================
   ADMIN & STAFF PORTAL HANDLERS - PHASE 4 (AUTH, RBAC, PAYMENTS, MAINTENANCE)
   ============================================================================ */

document.addEventListener('DOMContentLoaded', () => {
    checkAuthState();
    loadAdminDashboard();
});

function checkAuthState() {
    const user = API.getUser();
    const roleBadge = document.getElementById('portal-role-badge');
    const userDisplay = document.getElementById('user-display');
    const authNavItem = document.getElementById('auth-nav-item');
    const adminElements = document.querySelectorAll('.admin-only');

    if (user) {
        if (roleBadge) roleBadge.textContent = `[${user.role.toUpperCase()}]`;
        if (userDisplay) userDisplay.innerHTML = `Welcome, <strong>${user.username}</strong> (${user.role}) | <a href="#" onclick="handleStaffLogout()" style="color:#ef4444; text-decoration:none;">Logout</a>`;
        if (authNavItem) authNavItem.innerHTML = `<a href="#" onclick="handleStaffLogout()">Logout (${user.username})</a>`;

        if (user.role === 'receptionist') {
            adminElements.forEach(el => el.style.display = 'none');
        } else {
            adminElements.forEach(el => el.style.display = '');
        }
    } else {
        if (roleBadge) roleBadge.textContent = '[STAFF PORTAL]';
        if (userDisplay) userDisplay.innerHTML = `<a href="#" onclick="openModal('modal-login')" style="color:var(--gold-primary); text-decoration:none;">Click here to Login</a>`;
        if (authNavItem) authNavItem.innerHTML = `<a href="#" onclick="openModal('modal-login')">Login</a>`;
    }
}

async function handleStaffLogin(event) {
    event.preventDefault();
    const user = document.getElementById('login_user').value.trim();
    const pass = document.getElementById('login_pass').value.trim();

    try {
        const res = await API.login(user, pass);
        if (res.success) {
            alert(`Login Successful! Welcome ${res.username} (${res.role}).`);
            closeModal('modal-login');
            checkAuthState();
            loadAdminDashboard();
        } else {
            alert('Login Failed: ' + res.message);
        }
    } catch (err) {
        console.error(err);
        alert('Server login error.');
    }
}

async function handleStaffLogout() {
    if (confirm('Are you sure you want to log out?')) {
        await API.logout();
        checkAuthState();
        loadAdminDashboard();
    }
}

async function loadAdminDashboard() {
    loadKPIs();
    loadRoomsTable();
    loadBookingsTable();

    const user = API.getUser();
    if (user && user.role === 'admin') {
        loadAuditLogs();
    }
}

async function loadKPIs() {
    try {
        const res = await API.getStats();
        if (res.success && res.stats) {
            const s = res.stats;
            document.getElementById('kpi-total-rooms').textContent = s.total_rooms;
            document.getElementById('kpi-avail-rooms').textContent = s.available_rooms;
            document.getElementById('kpi-res-rooms').textContent = s.reserved_rooms;
            document.getElementById('kpi-occ-rooms').textContent = s.occupied_rooms;
            if (document.getElementById('kpi-occupancy')) {
                document.getElementById('kpi-occupancy').textContent = `${s.occupancy_rate ? s.occupancy_rate.toFixed(1) : 0}%`;
            }
            document.getElementById('kpi-revenue').textContent = `₹${s.total_revenue.toFixed(2)}`;
        }
    } catch (err) {
        console.error("Failed to load KPIs:", err);
    }
}

async function loadRoomsTable() {
    const tbody = document.getElementById('admin-rooms-table');
    if (!tbody) return;

    try {
        const res = await API.getRooms();
        if (!res.success || !res.rooms || res.rooms.length === 0) {
            tbody.innerHTML = '<tr><td colspan="4" style="text-align:center;">No rooms found.</td></tr>';
            return;
        }

        tbody.innerHTML = res.rooms.map(r => `
            <tr>
                <td><strong>Room ${r.room_no}</strong></td>
                <td>${r.type}</td>
                <td>₹${r.price.toFixed(2)}</td>
                <td><span class="badge badge-${r.status_str.toLowerCase()}">${r.status_str}</span></td>
            </tr>
        `).join('');
    } catch (err) {
        console.error("Failed to load rooms table:", err);
        tbody.innerHTML = '<tr><td colspan="4" style="text-align:center; color:red;">Error loading rooms.</td></tr>';
    }
}

async function loadBookingsTable() {
    const tbody = document.getElementById('admin-bookings-table');
    if (!tbody) return;

    try {
        const res = await API.getAllBookings();
        if (!res.success || !res.bookings || res.bookings.length === 0) {
            tbody.innerHTML = '<tr><td colspan="8" style="text-align:center;">No booking records found.</td></tr>';
            return;
        }

        tbody.innerHTML = res.bookings.map(b => `
            <tr>
                <td><strong style="color: var(--gold-primary);">${b.booking_id}</strong></td>
                <td>${b.customer_name}</td>
                <td>${b.phone}</td>
                <td>Room ${b.room_no}</td>
                <td>${b.days} nights</td>
                <td>${b.check_in}</td>
                <td>${b.check_out}</td>
                <td><span class="badge badge-${b.status_str.toLowerCase().replace('-', '')}">${b.status_str}</span></td>
            </tr>
        `).join('');
    } catch (err) {
        console.error("Failed to load bookings table:", err);
        tbody.innerHTML = '<tr><td colspan="8" style="text-align:center; color:red;">Error loading bookings.</td></tr>';
    }
}

async function loadAuditLogs() {
    const tbody = document.getElementById('admin-audit-table');
    if (!tbody) return;

    try {
        const res = await API.getAuditLogs();
        if (!res.success || !res.logs || res.logs.length === 0) {
            tbody.innerHTML = '<tr><td colspan="5" style="text-align:center;">No audit logs found.</td></tr>';
            return;
        }

        tbody.innerHTML = res.logs.map(l => `
            <tr>
                <td>#${l.log_id}</td>
                <td><small style="color:var(--text-muted);">${l.timestamp}</small></td>
                <td><strong>${l.username}</strong></td>
                <td><span style="color:var(--gold-primary);">${l.action}</span></td>
                <td>${l.details}</td>
            </tr>
        `).join('');
    } catch (err) {
        console.error("Failed to load audit logs:", err);
        tbody.innerHTML = '<tr><td colspan="5" style="text-align:center; color:red;">Error loading audit logs.</td></tr>';
    }
}

/* Modal Controls */
function openModal(id) {
    const el = document.getElementById(id);
    if (el) el.style.display = 'flex';
}

function closeModal(id) {
    const el = document.getElementById(id);
    if (el) el.style.display = 'none';
}

/* Actions */
async function handleAdminAddRoom(event) {
    event.preventDefault();
    const room_no = parseInt(document.getElementById('add_room_no').value);
    const type = document.getElementById('add_room_type').value;
    const price = parseFloat(document.getElementById('add_room_price').value);

    try {
        const res = await API.addRoom({ room_no, type, price });
        alert(res.message);
        if (res.success) {
            closeModal('modal-add-room');
            loadAdminDashboard();
        }
    } catch (err) {
        console.error(err);
        alert('Failed to add room.');
    }
}

async function handleAdminMaintenance(event) {
    event.preventDefault();
    const room_no = parseInt(document.getElementById('maint_room_no').value);
    const maintenance = parseInt(document.getElementById('maint_status').value);

    try {
        const res = await API.setMaintenance(room_no, maintenance);
        alert(res.message);
        if (res.success) {
            closeModal('modal-maintenance');
            loadAdminDashboard();
        }
    } catch (err) {
        console.error(err);
        alert('Failed to update maintenance status.');
    }
}

async function handleAdminCheckIn(event) {
    event.preventDefault();
    const bookingId = document.getElementById('checkin_id').value;

    try {
        const res = await API.checkIn(bookingId);
        alert(res.message);
        if (res.success) {
            closeModal('modal-checkin');
            loadAdminDashboard();
        }
    } catch (err) {
        console.error(err);
        alert('Failed to perform check-in.');
    }
}

async function handleAdminBilling(event) {
    event.preventDefault();
    const bookingId = document.getElementById('bill_id').value;
    const food = document.getElementById('bill_food').value;
    const service = document.getElementById('bill_service').value;

    try {
        const res = await API.generateBill(bookingId, food, service);
        if (res.success && res.bill) {
            alert(`Bill Calculated & Saved!\n\nRoom Charge: ₹${res.bill.room_charge.toFixed(2)}\nFood Charge: ₹${res.bill.food_charge.toFixed(2)}\nService Charge: ₹${res.bill.service_charge.toFixed(2)}\nTax (10%): ₹${res.bill.tax.toFixed(2)}\n-------------------------\nTOTAL: ₹${res.bill.total.toFixed(2)}`);
            closeModal('modal-billing');
            loadAdminDashboard();
        } else {
            alert(res.message);
        }
    } catch (err) {
        console.error(err);
        alert('Failed to calculate bill.');
    }
}

async function handleAdminPayment(event) {
    event.preventDefault();
    const bookingId = document.getElementById('pay_booking_id').value;
    const payMethod = document.getElementById('pay_method').value;

    try {
        const res = await API.recordPayment(bookingId, payMethod);
        alert(res.message);
        if (res.success) {
            closeModal('modal-payment');
            loadAdminDashboard();
        }
    } catch (err) {
        console.error(err);
        alert('Failed to record payment.');
    }
}

async function handleAdminCheckOut(event) {
    event.preventDefault();
    const bookingId = document.getElementById('checkout_id').value;

    try {
        const res = await API.checkOut(bookingId);
        alert(res.message);
        if (res.success) {
            closeModal('modal-checkout');
            loadAdminDashboard();
        }
    } catch (err) {
        console.error(err);
        alert('Failed to perform check-out.');
    }
}

async function handleAdminAddUser(event) {
    event.preventDefault();
    const username = document.getElementById('user_username').value.trim();
    const password = document.getElementById('user_password').value.trim();
    const role = document.getElementById('user_role').value;

    try {
        const res = await API.addUser({ username, password, role });
        alert(res.message);
        if (res.success) {
            closeModal('modal-add-user');
            loadAdminDashboard();
        }
    } catch (err) {
        console.error(err);
        alert('Failed to create user account.');
    }
}
