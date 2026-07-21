// ---------- Shared node config ----------
const NODES = ["ankle", "waist", "wrist"];
const NODE_COLORS = {
    ankle: "#4d7a9c",
    waist: "#ad7f3e",
    wrist: "#3c7d72",
};

// ---------- WebSocket connection ----------
const ws = new WebSocket("ws://192.168.4.1/ws");
const statusEl = document.getElementById("connection-status");
const connectionDot = document.getElementById("connection-dot");

ws.onopen = () => {
    statusEl.textContent = "CONNECTED";
    connectionDot.classList.add("connected");
};

ws.onclose = () => {
    statusEl.textContent = "DISCONNECTED";
    connectionDot.classList.remove("connected");
};

ws.onerror = () => {
    statusEl.textContent = "ERROR";
    connectionDot.classList.remove("connected");
};

ws.onmessage = (event) => {
    const msg = JSON.parse(event.data);

    if (msg.type === "imu") {
        handleImuMessage(msg);
    } else if (msg.type === "punch") {
        handlePunchMessage(msg);
    }
};

// ---------- Rolling gyro magnitude chart ----------
const MAX_POINTS = 100;

const gyroHistory = {
    ankle: [],
    waist: [],
    wrist: [],
};

const gyroChart = new Chart(document.getElementById("gyro-chart").getContext("2d"), {
    type: "line",
    data: {
        datasets: NODES.map((node) => ({
            label: node[0].toUpperCase() + node.slice(1),
            data: gyroHistory[node],
            borderColor: NODE_COLORS[node],
            backgroundColor: NODE_COLORS[node] + "22",
            borderWidth: 2,
            pointRadius: 0,
            tension: 0.25,
            fill: false,
        })),
    },
    options: {
        animation: false,
        parsing: false,
        normalized: true,
        responsive: true,
        maintainAspectRatio: false,
        interaction: { intersect: false },
        scales: {
            x: {
                type: "linear",
                display: false,
            },
            y: {
                beginAtZero: true,
                grid: { color: "#eef0f2" },
                ticks: { color: "#9aa0a6", font: { size: 10 } },
            },
        },
        plugins: {
            legend: { display: false },
            tooltip: { enabled: false },
        },
    },
});

let chartDirty = false;

function requestChartUpdate() {
    chartDirty = true;
}

function chartRenderLoop() {
    if (chartDirty) {
        gyroChart.update("none");
        chartDirty = false;
    }
    requestAnimationFrame(chartRenderLoop);
}
requestAnimationFrame(chartRenderLoop);

// ---------- 3D body visualization ----------
// Sensor positions sit on a boxing-stance humanoid: right leg/arm lead the
// stance so the ankle and wrist sensors land on real limb endpoints.
const NODE_POSITIONS = {
    ankle: new THREE.Vector3(0.16, -1.4, 0),
    waist: new THREE.Vector3(0, 0, 0),
    wrist: new THREE.Vector3(0.95, 0.35, 0.4),
};

const bodyVizBody = document.querySelector("#body-viz .panel-body");
const threeCanvas = document.getElementById("three-canvas");

const scene = new THREE.Scene();

const camera = new THREE.PerspectiveCamera(38, 1, 0.1, 100);
camera.position.set(2.8, 1.0, 3.7);
camera.lookAt(0, -0.15, 0);

const renderer = new THREE.WebGLRenderer({ canvas: threeCanvas, antialias: true, alpha: true });
renderer.setPixelRatio(window.devicePixelRatio);
renderer.setClearColor(0x000000, 0);

scene.add(new THREE.AmbientLight(0xffffff, 0.8));
const keyLight = new THREE.DirectionalLight(0xffffff, 0.55);
keyLight.position.set(3, 5, 4);
scene.add(keyLight);

const figureGroup = new THREE.Group();
scene.add(figureGroup);

// Low-poly humanoid: tapered cylinder "bones" capped with faceted joints so
// seams blend, all sharing one translucent steel-toned material for a
// clinical instrument look rather than a smooth toy figure.
const bodyMaterial = new THREE.MeshStandardMaterial({
    color: 0x555b63,
    roughness: 0.5,
    metalness: 0.18,
    transparent: true,
    opacity: 0.48,
    depthWrite: false,
});

function addBoneMesh(start, end, radiusStart, radiusEnd) {
    const direction = new THREE.Vector3().subVectors(end, start);
    const length = direction.length();
    const geometry = new THREE.CylinderGeometry(radiusEnd, radiusStart, length, 8);
    const mesh = new THREE.Mesh(geometry, bodyMaterial);
    mesh.position.copy(start).addScaledVector(direction, 0.5);
    mesh.quaternion.setFromUnitVectors(new THREE.Vector3(0, 1, 0), direction.clone().normalize());
    figureGroup.add(mesh);
    return mesh;
}

function addJointMesh(position, radius) {
    const mesh = new THREE.Mesh(new THREE.SphereGeometry(radius, 8, 6), bodyMaterial);
    mesh.position.copy(position);
    figureGroup.add(mesh);
    return mesh;
}

// Joint layout, figure facing +Z with the right side leading (boxing stance).
const pelvis = new THREE.Vector3(0, -0.15, 0);
const chest = new THREE.Vector3(0, 0.7, 0);
const neckBase = new THREE.Vector3(0, 0.9, 0);
const headCenter = new THREE.Vector3(0, 1.12, 0);

const rightShoulder = new THREE.Vector3(0.22, 0.68, 0);
const rightElbow = new THREE.Vector3(0.62, 0.5, 0.22);
const leftShoulder = new THREE.Vector3(-0.22, 0.68, 0);
const leftElbow = new THREE.Vector3(-0.3, 0.42, 0.05);
const leftWrist = new THREE.Vector3(-0.32, 0.02, 0.08);

const rightHip = new THREE.Vector3(0.14, -0.15, 0);
const rightKnee = new THREE.Vector3(0.16, -0.78, 0.08);
const leftHip = new THREE.Vector3(-0.14, -0.15, 0);
const leftKnee = new THREE.Vector3(-0.16, -0.78, -0.05);
const leftAnkle = new THREE.Vector3(-0.16, -1.4, 0);

// Torso, neck, head
addBoneMesh(pelvis, chest, 0.22, 0.19);
addJointMesh(pelvis, 0.2);
addJointMesh(chest, 0.17);
addBoneMesh(chest, neckBase, 0.09, 0.07);

// Faceted, slightly elongated head reads as a sensor pod rather than a
// smooth toy-like ball.
const headMesh = new THREE.Mesh(new THREE.IcosahedronGeometry(0.13, 0), bodyMaterial);
headMesh.position.copy(headCenter);
headMesh.scale.set(1, 1.2, 1);
figureGroup.add(headMesh);

// Arms (right arm extended into the wrist sensor, left arm guarding)
addJointMesh(rightShoulder, 0.09);
addBoneMesh(rightShoulder, rightElbow, 0.085, 0.07);
addJointMesh(rightElbow, 0.07);
addBoneMesh(rightElbow, NODE_POSITIONS.wrist, 0.07, 0.055);

addJointMesh(leftShoulder, 0.09);
addBoneMesh(leftShoulder, leftElbow, 0.085, 0.07);
addJointMesh(leftElbow, 0.07);
addBoneMesh(leftElbow, leftWrist, 0.07, 0.055);
addJointMesh(leftWrist, 0.06);

// Legs (right leg lead stance into the ankle sensor, left leg trailing)
addJointMesh(rightHip, 0.13);
addBoneMesh(rightHip, rightKnee, 0.125, 0.1);
addJointMesh(rightKnee, 0.1);
addBoneMesh(rightKnee, NODE_POSITIONS.ankle, 0.1, 0.08);

addJointMesh(leftHip, 0.13);
addBoneMesh(leftHip, leftKnee, 0.125, 0.1);
addJointMesh(leftKnee, 0.1);
addBoneMesh(leftKnee, leftAnkle, 0.1, 0.08);
addJointMesh(leftAnkle, 0.075);

const nodeMeshes = {};
NODES.forEach((node) => {
    const material = new THREE.MeshStandardMaterial({
        color: NODE_COLORS[node],
        emissive: new THREE.Color(NODE_COLORS[node]),
        emissiveIntensity: 0,
        roughness: 0.4,
    });
    const mesh = new THREE.Mesh(new THREE.SphereGeometry(0.13, 24, 24), material);
    mesh.position.copy(NODE_POSITIONS[node]);
    figureGroup.add(mesh);
    nodeMeshes[node] = { mesh, material, flashUntil: 0, emissive: 0 };
});

function flashNode(node) {
    const entry = nodeMeshes[node];
    if (!entry) return;
    entry.flashUntil = performance.now() + 350;
}

function resizeThree() {
    const width = bodyVizBody.clientWidth;
    const height = bodyVizBody.clientHeight;
    if (width === 0 || height === 0) return;
    camera.aspect = width / height;
    camera.updateProjectionMatrix();
    renderer.setSize(width, height, false);
}
new ResizeObserver(resizeThree).observe(bodyVizBody);
resizeThree();

function animateThree() {
    requestAnimationFrame(animateThree);

    const now = performance.now();
    NODES.forEach((node) => {
        const entry = nodeMeshes[node];
        const active = entry.flashUntil > now;
        const targetScale = active ? 1.7 : 1;
        const targetEmissive = active ? 1 : 0;
        entry.mesh.scale.setScalar(THREE.MathUtils.lerp(entry.mesh.scale.x, targetScale, 0.25));
        entry.emissive = THREE.MathUtils.lerp(entry.emissive, targetEmissive, 0.25);
        entry.material.emissiveIntensity = entry.emissive;
    });

    figureGroup.rotation.y += 0.0025;

    renderer.render(scene, camera);
}
animateThree();

// ---------- Message handlers ----------
function handleImuMessage(msg) {
    const series = gyroHistory[msg.node];
    if (series) {
        const magnitude = Math.sqrt(msg.gyro_x ** 2 + msg.gyro_y ** 2 + msg.gyro_z ** 2);
        series.push({ x: msg.timestamp, y: magnitude });
        if (series.length > MAX_POINTS) series.shift();
        requestChartUpdate();
    }

    if (msg.peak) {
        flashNode(msg.node);
    }
}

const PUNCH_LOG_MAX = 20;
const dt1El = document.getElementById("dt1-value");
const dt2El = document.getElementById("dt2-value");
const punchTypeBadge = document.getElementById("punch-type-badge");
const punchLogEntries = document.getElementById("punch-log-entries");

function handlePunchMessage(msg) {
    dt1El.textContent = msg.dt1.toFixed(1);
    dt2El.textContent = msg.dt2.toFixed(1);

    punchTypeBadge.textContent = msg.punch_type;
    punchTypeBadge.className = msg.valid ? "valid" : "invalid";

    const entry = document.createElement("div");
    entry.className = msg.valid ? "punch-entry" : "punch-entry invalid";
    const time = new Date().toLocaleTimeString([], { hour12: false });
    entry.innerHTML = `
        <span class="punch-time">${time}</span>
        <span class="punch-type">${msg.punch_type}</span>
        <span class="punch-dt">&Delta;t1 ${msg.dt1.toFixed(1)}ms &middot; &Delta;t2 ${msg.dt2.toFixed(1)}ms</span>
        <span class="punch-valid">${msg.valid ? "VALID" : "REJECTED"}</span>
    `;
    punchLogEntries.prepend(entry);

    while (punchLogEntries.children.length > PUNCH_LOG_MAX) {
        punchLogEntries.removeChild(punchLogEntries.lastChild);
    }
}
