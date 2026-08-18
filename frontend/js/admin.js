/* ============================================================================
   ADMIN DASHBOARD HANDLERS & MODALS JAVASCRIPT
   ============================================================================ */

document.addEventListener('DOMContentLoaded', () => {
    loadAdminDashboard();
});

async function loadAdminDashboard() {
    loadKPIs();
    loadRoomsTable();
    loadBookingsTable();
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
                <td><strong style="color: var(--gold-accent);">${b.booking_id}</strong></td>
                <td>${b.customer_name}</td>
                <td>${b.phone}</td>
                <td>Room ${b.room_no}</td>
                <td>${b.days} days</td>
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

/* Modal Helpers */
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
        alert('Failed to add room via C backend server.');
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
