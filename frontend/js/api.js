/* ============================================================================
   CLIENT API WRAPPER - PHASE 4 (AUTH, RBAC, PAYMENTS, MAINTENANCE, LOGS)
   ============================================================================ */

const API_BASE_URL = 'http://127.0.0.1:8080/api';

const API = {
    // Session token management
    getToken() {
        return localStorage.getItem('session_token') || '';
    },
    setToken(token) {
        if (token) localStorage.setItem('session_token', token);
        else localStorage.removeItem('session_token');
    },
    getUser() {
        const u = localStorage.getItem('user_info');
        return u ? JSON.parse(u) : null;
    },
    setUser(user) {
        if (user) localStorage.setItem('user_info', JSON.stringify(user));
        else localStorage.removeItem('user_info');
    },

    // Base request helper with X-Session-Token header
    async request(endpoint, options = {}) {
        const url = `${API_BASE_URL}${endpoint}`;
        const headers = {
            'Content-Type': 'application/json',
            ...(options.headers || {})
        };

        const token = this.getToken();
        if (token) {
            headers['X-Session-Token'] = token;
        }

        const config = {
            ...options,
            headers
        };

        try {
            const response = await fetch(url, config);
            const data = await response.json();
            return data;
        } catch (error) {
            console.error(`API Error on [${endpoint}]:`, error);
            return { success: false, message: 'Server connection failed. Please ensure C server is running on http://127.0.0.1:8080.' };
        }
    },

    // Auth
    async login(username, password) {
        const res = await this.request('/login', {
            method: 'POST',
            body: JSON.stringify({ username, password })
        });
        if (res.success) {
            this.setToken(res.token);
            this.setUser({ user_id: res.user_id, username: res.username, role: res.role });
        }
        return res;
    },
    async logout() {
        const res = await this.request('/logout', { method: 'POST' });
        this.setToken('');
        this.setUser(null);
        return res;
    },

    // Rooms
    async getRooms() { return await this.request('/rooms'); },
    async getAvailableRooms() { return await this.request('/rooms/available'); },
    async addRoom(roomData) {
        return await this.request('/rooms/add', { method: 'POST', body: JSON.stringify(roomData) });
    },
    async setMaintenance(room_no, maintenance) {
        return await this.request('/rooms/maintenance', { method: 'POST', body: JSON.stringify({ room_no, maintenance }) });
    },

    // Bookings
    async getAllBookings() { return await this.request('/bookings'); },
    async createBooking(bookingData) {
        return await this.request('/bookings', { method: 'POST', body: JSON.stringify(bookingData) });
    },
    async lookupBooking(booking_id, phone) {
        return await this.request('/bookings/lookup', { method: 'POST', body: JSON.stringify({ booking_id: parseInt(booking_id), phone }) });
    },
    async cancelBooking(booking_id, phone) {
        return await this.request('/bookings/cancel', { method: 'POST', body: JSON.stringify({ booking_id: parseInt(booking_id), phone }) });
    },

    // Check-In / Check-Out / Bills / Payments
    async checkIn(booking_id) {
        return await this.request('/checkin', { method: 'POST', body: JSON.stringify({ booking_id: parseInt(booking_id) }) });
    },
    async generateBill(booking_id, food_charge, service_charge) {
        return await this.request('/bills', {
            method: 'POST',
            body: JSON.stringify({ booking_id: parseInt(booking_id), food_charge: parseFloat(food_charge), service_charge: parseFloat(service_charge) })
        });
    },
    async recordPayment(booking_id, payment_method) {
        return await this.request('/payments', {
            method: 'POST',
            body: JSON.stringify({ booking_id: parseInt(booking_id), payment_method })
        });
    },
    async checkOut(booking_id) {
        return await this.request('/checkout', { method: 'POST', body: JSON.stringify({ booking_id: parseInt(booking_id) }) });
    },

    // Users & Audit Logs (Admin)
    async getUsers() { return await this.request('/users'); },
    async addUser(userData) {
        return await this.request('/users/add', { method: 'POST', body: JSON.stringify(userData) });
    },
    async getAuditLogs() { return await this.request('/audit_logs'); },

    // Stats & KPIs
    async getStats() { return await this.request('/stats'); }
};
