/* ============================================================================
   BOOKING HANDLER & CUSTOMER LOOKUP JAVASCRIPT
   ============================================================================ */

document.addEventListener('DOMContentLoaded', () => {
    // If on booking.html, load available rooms dropdown
    if (document.getElementById('select_room')) {
        initBookingForm();
    }

    // If on booking-confirmation.html, load receipt details
    if (document.getElementById('v-booking-id') || document.getElementById('voucher-id')) {
        loadBookingConfirmation();
    }
});

let roomsMap = {};

async function initBookingForm() {
    const select = document.getElementById('select_room');
    const urlParams = new URLSearchParams(window.location.search);
    const preselectedRoom = urlParams.get('room_no');

    // Default dates (today & +3 days)
    const today = new Date();
    const future = new Date();
    future.setDate(today.getDate() + 3);

    const inInput = document.getElementById('check_in_date');
    const outInput = document.getElementById('check_out_date');

    if (inInput) inInput.value = formatDateForInput(today);
    if (outInput) outInput.value = formatDateForInput(future);

    try {
        const res = await API.getAvailableRooms();
        if (!res.success || !res.rooms || res.rooms.length === 0) {
            select.innerHTML = '<option value="">No available rooms for booking</option>';
            return;
        }

        select.innerHTML = '<option value="">-- Select a Room --</option>' + res.rooms.map(room => {
            roomsMap[room.room_no] = room;
            const isSelected = preselectedRoom && room.room_no == preselectedRoom ? 'selected' : '';
            return `<option value="${room.room_no}" ${isSelected}>Room ${room.room_no} (${room.type}) - ₹${room.price.toFixed(2)}/night</option>`;
        }).join('');

        updateEstimatedCost();
    } catch (err) {
        console.error(err);
        select.innerHTML = '<option value="">Error loading rooms from server</option>';
    }
}

function formatDateForInput(date) {
    const year = date.getFullYear();
    const month = String(date.getMonth() + 1).padStart(2, '0');
    const day = String(date.getDate()).padStart(2, '0');
    return `${year}-${month}-${day}`;
}

function updateEstimatedCost() {
    const roomNo = document.getElementById('select_room')?.value;
    const inDateStr = document.getElementById('check_in_date')?.value;
    const outDateStr = document.getElementById('check_out_date')?.value;

    const previewDays = document.getElementById('preview-days');
    const previewTotal = document.getElementById('preview-total');

    if (!previewDays || !previewTotal) return;

    if (!roomNo || !inDateStr || !outDateStr) {
        previewDays.textContent = '0 nights';
        previewTotal.textContent = '₹0.00';
        return;
    }

    const inDate = new Date(inDateStr);
    const outDate = new Date(outDateStr);
    const diffTime = outDate - inDate;
    const diffDays = Math.ceil(diffTime / (1000 * 60 * 60 * 24));

    if (isNaN(diffDays) || diffDays <= 0) {
        previewDays.textContent = 'Invalid Dates';
        previewTotal.textContent = '₹0.00';
        return;
    }

    const room = roomsMap[roomNo];
    const price = room ? room.price : 0;
    const total = price * diffDays;

    previewDays.textContent = `${diffDays} nights`;
    previewTotal.textContent = `₹${total.toFixed(2)}`;
}

async function submitBooking(event) {
    event.preventDefault();
    const errorDiv = document.getElementById('booking-error');
    const submitBtn = document.getElementById('submit-btn');
    if (errorDiv) errorDiv.style.display = 'none';

    const room_no = parseInt(document.getElementById('select_room').value);
    const customer_name = document.getElementById('customer_name').value.trim();
    const phone = document.getElementById('phone').value.trim();
    const email = document.getElementById('email').value.trim();
    const check_in_date = document.getElementById('check_in_date').value;
    const check_out_date = document.getElementById('check_out_date').value;
    const guests = parseInt(document.getElementById('guests_count').value);

    if (!room_no || !customer_name || !phone || !check_in_date || !check_out_date) {
        if (errorDiv) {
            errorDiv.textContent = 'Please fill in all required fields.';
            errorDiv.style.display = 'block';
        }
        return;
    }

    if (submitBtn) {
        submitBtn.disabled = true;
        submitBtn.textContent = 'Processing Booking...';
    }

    try {
        const payload = {
            room_no,
            customer_name,
            phone,
            email,
            check_in_date,
            check_out_date,
            guests
        };

        const res = await API.createBooking(payload);
        if (res.success) {
            localStorage.setItem('last_booking', JSON.stringify({
                booking_id: res.booking_id,
                customer_name,
                phone,
                room_no,
                room_type: res.room_type || 'Deluxe',
                days: res.days,
                check_in_date,
                check_out_date,
                room_charge: res.room_charge
            }));
            window.location.href = `booking-confirmation.html?booking_id=${res.booking_id}&phone=${encodeURIComponent(phone)}`;
        } else {
            if (errorDiv) {
                errorDiv.textContent = res.message || 'Failed to complete booking.';
                errorDiv.style.display = 'block';
            }
            if (submitBtn) {
                submitBtn.disabled = false;
                submitBtn.textContent = 'CONFIRM RESERVATION';
            }
        }
    } catch (err) {
        console.error(err);
        if (errorDiv) {
            errorDiv.textContent = 'Server connection error. Please ensure C server is running on http://127.0.0.1:8080.';
            errorDiv.style.display = 'block';
        }
        if (submitBtn) {
            submitBtn.disabled = false;
            submitBtn.textContent = 'CONFIRM RESERVATION';
        }
    }
}

async function loadBookingConfirmation() {
    const urlParams = new URLSearchParams(window.location.search);
    const bookingId = urlParams.get('booking_id');
    const phone = urlParams.get('phone');

    if (bookingId && phone) {
        try {
            const res = await API.lookupBooking(bookingId, phone);
            if (res.success && res.booking) {
                const b = res.booking;
                if (document.getElementById('v-booking-id')) document.getElementById('v-booking-id').textContent = `Booking ID: #${b.booking_id}`;
                if (document.getElementById('v-guest-name')) document.getElementById('v-guest-name').textContent = b.customer_name;
                if (document.getElementById('v-phone')) document.getElementById('v-phone').textContent = b.phone;
                if (document.getElementById('v-room')) document.getElementById('v-room').textContent = `Room ${b.room_no}`;
                if (document.getElementById('v-room-type')) document.getElementById('v-room-type').textContent = b.room_type || 'Standard';
                if (document.getElementById('v-in-date')) document.getElementById('v-in-date').textContent = b.check_in;
                if (document.getElementById('v-out-date')) document.getElementById('v-out-date').textContent = b.check_out;
                if (document.getElementById('v-days')) document.getElementById('v-days').textContent = `${b.days} nights`;
                if (document.getElementById('v-total')) document.getElementById('v-total').textContent = `₹${b.total_billed ? b.total_billed.toFixed(2) : '0.00'}`;
            }
        } catch (err) {
            console.error("Lookup voucher error:", err);
        }
    }
}

async function handleBookingLookup(event) {
    event.preventDefault();
    const bookingId = document.getElementById('lookup_id').value.trim();
    const phone = document.getElementById('lookup_phone').value.trim();

    if (!bookingId || !phone) {
        alert('Please enter both Booking ID and Phone Number.');
        return;
    }

    try {
        const res = await API.lookupBooking(bookingId, phone);
        if (res.success && res.booking) {
            const b = res.booking;
            document.getElementById('det-id').textContent = `#${b.booking_id}`;
            document.getElementById('det-name').textContent = b.customer_name;
            document.getElementById('det-phone').textContent = b.phone;
            document.getElementById('det-room').textContent = `Room ${b.room_no} (${b.room_type || 'Standard'})`;
            document.getElementById('det-in').textContent = b.check_in;
            document.getElementById('det-out').textContent = b.check_out;

            const badge = document.getElementById('det-status');
            if (badge) {
                badge.textContent = b.status_str;
                badge.className = `badge badge-${b.status_str.toLowerCase().replace('-', '')}`;
            }

            const card = document.getElementById('booking-details-card');
            if (card) card.style.display = 'block';

            const cancelBtn = document.getElementById('cancel-btn');
            if (cancelBtn) {
                if (b.status === 1) { // Reserved
                    cancelBtn.style.display = 'block';
                } else {
                    cancelBtn.style.display = 'none';
                }
            }
        } else {
            alert(res.message || 'No matching reservation found.');
        }
    } catch (err) {
        console.error(err);
        alert('Error communicating with C backend server.');
    }
}

async function handleBookingCancel() {
    const bookingId = document.getElementById('lookup_id').value.trim();
    const phone = document.getElementById('lookup_phone').value.trim();

    if (!confirm(`Are you sure you want to cancel Reservation #${bookingId}?`)) {
        return;
    }

    try {
        const res = await API.cancelBooking(bookingId, phone);
        alert(res.message);
        if (res.success) {
            window.location.reload();
        }
    } catch (err) {
        console.error(err);
        alert('Error cancelling booking.');
    }
}
