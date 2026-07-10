// WebSocket connection
const ws = new WebSocket("ws://192.168.4.1/ws");
const statusEl = document.getElementById("connection-status");

ws.onopen = () => {
    statusEl.textContent = "CONNECTED";
    statusEl.style.color = "lime";
};

ws.onclose = () => {
    statusEl.textContent = "DISCONNECTED";
    statusEl.style.color = "red";
};

ws.onmessage = (event) => {
    const msg = JSON.parse(event.data);
    
    if (msg.type === "imu") {
        handleImuMessage(msg);
    } else if (msg.type === "punch") {
        handlePunchMessage(msg);
    }
};

function handleImuMessage(msg) {
    console.log("IMU:", msg.node, msg.gyro_x, msg.gyro_y, msg.gyro_z);
    // Chart/3D updates go here later
}

function handlePunchMessage(msg) {
    console.log("PUNCH:", msg.dt1, msg.dt2);
    
    document.getElementById("dt1-value").textContent = msg.dt1.toFixed(1);
    document.getElementById("dt2-value").textContent = msg.dt2.toFixed(1);
}