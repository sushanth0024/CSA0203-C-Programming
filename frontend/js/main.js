/* ============================================================================
   MAIN HOMEPAGE & UTILITY JAVASCRIPT
   ============================================================================ */

document.addEventListener('DOMContentLoaded', () => {
    // Set default search dates (today & 3 days later)
    const today = new Date();
    const futureDate = new Date();
    futureDate.setDate(today.getDate() + 3);

    const checkinInput = document.getElementById('checkin');
    const checkoutInput = document.getElementById('checkout');

    if (checkinInput && checkoutInput) {
        checkinInput.value = formatDateForInput(today);
        checkoutInput.value = formatDateForInput(futureDate);
    }

    // Load rooms if on index.html or rooms.html
    loadFeaturedRooms();
});

function formatDateForInput(date) {
    const year = date.getFullYear();
    const month = String(date.getMonth() + 1).padStart(2, '0');
    const day = String(date.getDate()).padStart(2, '0');
    return `${year}-${month}-${day}`;
}

async function loadFeaturedRooms() {
    const grid = document.getElementById('featured-rooms-grid');
    if (!grid) return;

    try {
        const res = await API.getAvailableRooms();
        if (!res.success || !res.rooms || res.rooms.length === 0) {
            grid.innerHTML = '<p style="text-align:center; grid-column: 1/-1;">No rooms currently available for reservation.</p>';
            return;
        }

        grid.innerHTML = res.rooms.map(room => `
            <div class="room-card">
                <div class="room-card-img" style="background-image: url('${getRoomImage(room.type)}');">
                    <span class="room-badge">${room.type} Class</span>
                </div>
                <div class="room-card-body">
                    <h3 class="room-title">Room ${room.room_no} (${room.type})</h3>
                    <div class="room-price">₹${room.price.toFixed(2)} <span>/ night</span></div>
                    <ul class="room-features">
                        <li>✔️ King/Queen Size Bed</li>
                        <li>✔️ Free High-Speed Wi-Fi & Smart TV</li>
                        <li>✔️ Air Conditioned & Room Service</li>
                        <li>✔️ Status: <span class="badge badge-available">AVAILABLE</span></li>
                    </ul>
                    <a href="booking.html?room_no=${room.room_no}" class="btn-primary" style="text-align: center; text-decoration: none; display: block;">BOOK NOW</a>
                </div>
            </div>
        `).join('');
    } catch (err) {
        console.error("Failed to load rooms:", err);
        grid.innerHTML = '<p style="text-align:center; grid-column: 1/-1; color: red;">Error connecting to C Server at http://127.0.0.1:8080. Please verify backend server is running.</p>';
    }
}

function getRoomImage(type) {
    if (type.toLowerCase().includes('single')) {
        return 'https://images.unsplash.com/photo-1631049307264-da0ec9d70304?auto=format&fit=crop&w=600&q=80';
    } else if (type.toLowerCase().includes('deluxe')) {
        return 'https://images.unsplash.com/photo-1582719478250-c89cae4dc85b?auto=format&fit=crop&w=600&q=80';
    } else {
        return 'https://images.unsplash.com/photo-1566665797739-1674de7a421a?auto=format&fit=crop&w=600&q=80';
    }
}

function handleSearchRooms() {
    window.location.href = 'rooms.html';
}
