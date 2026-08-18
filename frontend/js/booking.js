/* ============================================================================
   BOOKING HANDLER & CUSTOMER LOOKUP JAVASCRIPT
   ============================================================================ */

document.addEventListener('DOMContentLoaded', () => {
    // If on booking.html, load available rooms dropdown
    if (document.getElementById('select_room')) {
        initBookingForm();
    }

    // If on booking-confirmation.html, load receipt details
    if (document.getElementById('voucher-id')) {
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

    document.getElementById('check_in_date').value = formatDateForInput(today);
    document.getElementById('check_out_date').value = formatDateForInput(future);

    try {
        const res = await API.getAvailableRooms();
        if (!res.success || !res.rooms || res.rooms.length === 0) {
            select.innerHTML = '<option value="">No rooms available</option>';
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
        select.innerHTML = '<option value="">Error loading rooms</option>';
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
        previewDays.textContent = '0';
        previewTotal.textContent = '₹0.00';
        return;
    }

    const inDate = new Date(inDateStr);
    const outDate = new Date(outDateStr);
    const diffTime = outDate - inDate;
    const diffDays = Math.ceil(diffTime / (1000 * 60 * 60 * 24));

    if (diffDays <= 0) {
        previewDays.textContent = 'Invalid Dates';
        previewTotal.textContent = '₹0.00';
        return;
    }

    const room = roomsMap[roomNo];
    const price = room ? room.price : 0;
    const total = price * diffDays;

    previewDays.textContent = diffDays;
    previewTotal.textContent = `₹${total.toFixed(2)}`;
}

async function submitBooking(event) {
    event.preventDefault();
    const errorDiv = document.getElementById('booking-error');
    const submitBtn = document.getElementById('submit-btn');
    errorDiv.style.display = 'none';

    const room_no = parseInt(document.getElementById('select_room').value);
    const customer_name = document.getElementById('customer_name').value.trim();
    const phone = document.getElementById('phone').value.trim();
    const email = document.getElementById('email').value.trim();
    const check_in_date = document.getElementById('check_in_date').value;
    const check_out_date = document.getElementById('check_out_date').value;
    const guests = parseInt(document.getElementById('guests_count').value);

    if (!room_no || !customer_name || !phone || !check_in_date || !check_out_date) {
        errorDiv.textContent = 'Please fill in all required fields.';
        errorDiv.style.display = 'block';
        return;
    }

    submitBtn.disabled = true;
    submitBtn.textContent = 'Processing Booking...';

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
            // Redirect to voucher confirmation page
            window.location.href = `booking-confirmation.html?booking_id=${res.booking_id}&phone=${encodeURIComponent(phone)}`;
        } else {
            errorDiv.textContent = res.message || 'Failed to complete booking.';
            errorDiv.style.display = 'block';
            submitBtn.disabled = false;
            submitBtn.textContent = 'CONFIRM BOOKING';
        }
    } catch (err) {
        console.error(err);
        errorDiv.textContent = 'Server connection error. Please ensure C server is running on http://127.0.0.1:8080.';
        errorDiv.style.display = 'block';
        submitBtn.disabled = false;
        submitBtn.textContent = 'CONFIRM BOOKING';
    }
}

async function loadBookingConfirmation() {
    const urlParams = new URLSearchParams(window.location.search);
    const bookingId = urlParams.get('booking_id');
    const phone = urlParams.get('phone');

    if (!bookingId || !phone) {
        alert('Invalid voucher request.');
        window.location.href = 'index.html';
        return;
    }

    try {
        const res = await API.lookupBooking(bookingId, phone);
        if (!res.success || !res.booking) {
            alert('Booking voucher not found.');
            window.location.href = 'index.html';
            return;
        }

        const b = res.booking;
        document.getElementById('voucher-id').textContent = b.booking_id;
        document.getElementById('voucher-name').textContent = b.customer_name;
        document.getElementById('voucher-phone').textContent = b.phone;
        document.getElementById('voucher-room').textContent = `Room ${b.room_no} (${b.room_type})`;
        document.getElementById('voucher-checkin').textContent = b.check_in;
        document.getElementById('voucher-checkout').textContent = b.check_out;
        document.getElementById('voucher-days').textContent = b.days;
        document.getElementById('voucher-guests').textContent = b.guests;
        document.getElementById('voucher-total').textContent = `₹${b.total_billed.toFixed(2)}`;
        document.getElementById('voucher-status').textContent = b.status_str;
    } catch (err) {
        console.error(err);
        alert('Error retrieving voucher from C backend server.');
    }
}

async function searchCustomerBooking(event) {
    event.preventDefault();
    const resultDiv = document.getElementById('search-result');
    const errorDiv = document.getElementById('search-error');
    errorDiv.style.display = 'none';
    resultDiv.style.display = 'none';

    const bookingId = document.getElementById('lookup_id').value.trim();
    const phone = document.getElementById('lookup_phone').value.trim();

    if (!bookingId || !phone) {
        errorDiv.textContent = 'Please enter both Booking ID and Phone Number.';
        errorDiv.style.display = 'block';
        return;
    }

    try {
        const res = await API.lookupBooking(bookingId, phone);
        if (!res.success || !res.booking) {
            errorDiv.textContent = res.message || 'No matching booking found.';
            errorDiv.style.display = 'block';
            return;
        }

        const b = res.booking;
        document.getElementById('lookup-id').textContent = b.booking_id;
        document.getElementById('lookup-name').textContent = b.customer_name;
        document.getElementById('lookup-phone').textContent = b.phone;
        document.getElementById('lookup-room').textContent = `Room ${b.room_no} (${b.room_type})`;
        document.getElementById('lookup-checkin').textContent = b.check_in;
        document.getElementById('lookup-checkout').textContent = b.check_out;
        document.getElementById('lookup-days').textContent = b.days;
        document.getElementById('lookup-total').textContent = `₹${b.total_billed.toFixed(2)}`;

        const badgeSpan = document.getElementById('lookup-status-badge');
        badgeSpan.textContent = b.status_str;
        badgeSpan.className = `badge badge-${b.status_str.toLowerCase().replace('-', '')}`;

        const cancelBtn = document.getElementById('cancel-res-btn');
        if (b.status === 1) { // Reserved
            cancelBtn.style.display = 'block';
            cancelBtn.onclick = () => performCancellation(b.booking_id, b.phone);
        } else {
            cancelBtn.style.display = 'none';
        }

        resultDiv.style.display = 'block';
    } catch (err) {
        console.error(err);
        errorDiv.textContent = 'Failed to connect to C backend server.';
        errorDiv.style.display = 'block';
    }
}

async function performCancellation(bookingId, phone) {
    if (!confirm(`Are you sure you want to cancel Reservation ID ${bookingId}?`)) {
        return;
    }

    try {
        const res = await API.cancelBooking(bookingId, phone);
        if (res.success) {
            alert('Reservation cancelled successfully! Room is now Available.');
            window.location.reload();
        } else {
            alert('Cancellation failed: ' + res.message);
        }
    } catch (err) {
        console.error(err);
        alert('Error communicating with C backend server.');
    }
}
