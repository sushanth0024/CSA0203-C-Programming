/* ============================================================================
   FRONTEND API CLIENT WRAPPER - CONNECTS TO C BACKEND
   ============================================================================ */
const API_BASE = "http://127.0.0.1:8080/api";

const API = {
    // Rooms APIs
    getRooms: async () => {
        const res = await fetch(`${API_BASE}/rooms`);
        return res.json();
    },

    getAvailableRooms: async () => {
        const res = await fetch(`${API_BASE}/rooms/available`);
        return res.json();
    },

    addRoom: async (roomData) => {
        const res = await fetch(`${API_BASE}/rooms/add`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(roomData)
        });
        return res.json();
    },

    // Booking APIs
    createBooking: async (bookingData) => {
        const res = await fetch(`${API_BASE}/bookings`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(bookingData)
        });
        return res.json();
    },

    lookupBooking: async (bookingId, phone) => {
        const res = await fetch(`${API_BASE}/bookings/lookup`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ booking_id: parseInt(bookingId), phone: phone })
        });
        return res.json();
    },

    cancelBooking: async (bookingId, phone) => {
        const res = await fetch(`${API_BASE}/bookings/cancel`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ booking_id: parseInt(bookingId), phone: phone })
        });
        return res.json();
    },

    // Admin APIs
    getAllBookings: async () => {
        const res = await fetch(`${API_BASE}/bookings`);
        return res.json();
    },

    checkIn: async (bookingId) => {
        const res = await fetch(`${API_BASE}/checkin`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ booking_id: parseInt(bookingId) })
        });
        return res.json();
    },

    getBill: async (bookingId) => {
        const res = await fetch(`${API_BASE}/bills/${bookingId}`);
        return res.json();
    },

    generateBill: async (bookingId, foodCharge, serviceCharge) => {
        const res = await fetch(`${API_BASE}/bills`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({
                booking_id: parseInt(bookingId),
                food_charge: parseFloat(foodCharge || 0),
                service_charge: parseFloat(serviceCharge || 0)
            })
        });
        return res.json();
    },

    checkOut: async (bookingId) => {
        const res = await fetch(`${API_BASE}/checkout`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ booking_id: parseInt(bookingId) })
        });
        return res.json();
    },

    getStats: async () => {
        const res = await fetch(`${API_BASE}/stats`);
        return res.json();
    }
};
