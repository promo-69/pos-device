#include "CommsHandler.h"
#include "NetworkManager.h" 
#include "PaymentBuffer.h"
#include "POSController.h"

QueueHandle_t orderQueue = NULL;
CommsHandler* globalComms = nullptr;

// Dashboard SPA completo guardado directamente en la memoria Flash (PROGMEM)
const char dashboardHtml[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="es">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 POS Dashboard</title>
    <style>
        :root {
            --bg-color: #0f172a; --glass-bg: rgba(30, 41, 59, 0.7);
            --glass-border: rgba(255, 255, 255, 0.1); --primary: #3b82f6;
            --primary-hover: #2563eb; --text-main: #f8fafc; --text-muted: #94a3b8;
            --danger: #ef4444; --success: #10b981; --warning: #f59e0b;
        }
        * { box-sizing: border-box; font-family: 'Segoe UI', Roboto, Helvetica, Arial, sans-serif; }
        body {
            margin: 0; padding: 0; background-color: var(--bg-color);
            background-image: radial-gradient(circle at top right, #1e1b4b, transparent 40%), radial-gradient(circle at bottom left, #064e3b, transparent 40%);
            background-attachment: fixed; color: var(--text-main); display: flex; min-height: 100vh;
        }
        /* Sidebar */
        .sidebar { width: 260px; background: var(--glass-bg); backdrop-filter: blur(16px); border-right: 1px solid var(--glass-border); padding: 2rem 1rem; display: flex; flex-direction: column; gap: 1rem; }
        .sidebar h2 { margin: 0 0 2rem 0; font-size: 1.5rem; text-align: center; background: linear-gradient(to right, #60a5fa, #a78bfa); -webkit-background-clip: text; -webkit-text-fill-color: transparent; }
        .nav-btn { background: transparent; color: var(--text-muted); border: none; padding: 1rem; text-align: left; border-radius: 10px; cursor: pointer; font-size: 1rem; transition: all 0.3s; }
        .nav-btn:hover { background: rgba(255,255,255,0.05); color: var(--text-main); }
        .nav-btn.active { background: rgba(59,130,246,0.2); color: var(--primary); border: 1px solid rgba(59,130,246,0.3); }
        /* Main */
        .main-content { flex: 1; padding: 2rem; overflow-y: auto; }
        .view { display: none; animation: fadeIn 0.3s ease; }
        .view.active { display: block; }
        @keyframes fadeIn { from { opacity: 0; transform: translateY(10px); } to { opacity: 1; transform: translateY(0); } }
        /* Cards & Forms */
        .card { background: var(--glass-bg); backdrop-filter: blur(16px); border: 1px solid var(--glass-border); border-radius: 20px; padding: 2rem; max-width: 600px; margin: 0 auto; }
        .form-group { margin-bottom: 1.5rem; }
        label { display: block; margin-bottom: 0.5rem; font-size: 0.85rem; text-transform: uppercase; color: var(--text-muted); }
        input[type="text"], input[type="number"] { width: 100%; padding: 0.75rem 1rem; background: rgba(15, 23, 42, 0.6); border: 1px solid var(--glass-border); border-radius: 10px; color: var(--text-main); font-size: 1rem; transition: all 0.3s; }
        input:focus { outline: none; border-color: var(--primary); }
        button.btn-primary { width: 100%; padding: 0.85rem; background: var(--primary); color: white; border: none; border-radius: 10px; font-size: 1rem; cursor: pointer; transition: 0.3s; }
        button.btn-primary:hover { background: var(--primary-hover); }
        button.btn-danger { width: 100%; padding: 0.85rem; background: rgba(220, 38, 38, 0.1); color: var(--danger); border: 1px solid rgba(239, 68, 68, 0.2); border-radius: 10px; font-size: 1rem; cursor: pointer; transition: 0.3s; margin-top: 1rem; }
        button.btn-danger:hover { background: rgba(220, 38, 38, 0.2); }
        /* Status UI */
        .status-box { margin-top: 2rem; padding: 1.5rem; border-radius: 10px; text-align: center; display: none; }
        .status-box.EN_COLA, .status-box.PROCESANDO { background: rgba(245, 158, 11, 0.1); border: 1px solid var(--warning); color: var(--warning); display: block; }
        .status-box.COMPLETADO { background: rgba(16, 185, 129, 0.1); border: 1px solid var(--success); color: var(--success); display: block; }
        .status-box.ERROR { background: rgba(239, 68, 68, 0.1); border: 1px solid var(--danger); color: var(--danger); display: block; }
        .loader { border: 3px solid rgba(255,255,255,0.1); border-top: 3px solid var(--warning); border-radius: 50%; width: 24px; height: 24px; animation: spin 1s linear infinite; margin: 0 auto 10px auto; }
        @keyframes spin { 0% { transform: rotate(0deg); } 100% { transform: rotate(360deg); } }
        /* Table */
        .table-container { width: 100%; overflow-x: auto; margin-top: 1rem; }
        table { width: 100%; border-collapse: collapse; text-align: left; }
        th, td { padding: 1rem; border-bottom: 1px solid var(--glass-border); }
        th { color: var(--text-muted); font-size: 0.85rem; text-transform: uppercase; }
        .badge { padding: 0.25rem 0.75rem; border-radius: 9999px; font-size: 0.75rem; font-weight: bold; }
        .badge.EN_COLA { background: rgba(245,158,11,0.2); color: var(--warning); }
        .badge.PROCESANDO { background: rgba(59,130,246,0.2); color: var(--primary); }
        .badge.COMPLETADO { background: rgba(16,185,129,0.2); color: var(--success); }
        .badge.ERROR { background: rgba(239,68,68,0.2); color: var(--danger); }
        .top-bar { display: flex; justify-content: space-between; align-items: center; margin-bottom: 1rem; }
        .search-box { display: flex; gap: 0.5rem; }
        .search-box input { width: 250px; }
        .btn-small { padding: 0.5rem 1rem; background: var(--glass-bg); color: var(--text-main); border: 1px solid var(--glass-border); border-radius: 8px; cursor: pointer; }
        .btn-small:hover { background: rgba(255,255,255,0.1); }
        /* Mobile */
        @media (max-width: 768px) {
            body { flex-direction: column; }
            .sidebar { width: 100%; padding: 1rem; border-right: none; border-bottom: 1px solid var(--glass-border); flex-direction: row; flex-wrap: wrap; justify-content: center; gap: 0.5rem; }
            .sidebar h2 { width: 100%; margin-bottom: 0.5rem; }
            .nav-btn { flex: 1; text-align: center; padding: 0.5rem; font-size: 0.9rem; }
            .top-bar { flex-direction: column; gap: 1rem; align-items: stretch; }
            .search-box input { width: 100%; }
        }
    </style>
</head>
<body>
    <div class="sidebar">
        <h2>ESP32 POS</h2>
        <button class="nav-btn active" onclick="switchView('pagos')">🛒 Realizar Pago</button>
        <button class="nav-btn" onclick="switchView('historial')">📋 Consultar Pagos</button>
        <button class="nav-btn" onclick="switchView('lector')">🏷️ Lector Tarjetas</button>
        <button class="nav-btn" onclick="switchView('config')">⚙️ Configuración</button>
    </div>
    <div class="main-content">
        <!-- VISTA: PAGOS -->
        <div id="pagos" class="view active">
            <div class="card">
                <h3 style="margin-top:0;">Nuevo Cobro</h3>
                <form id="payForm">
                    <div class="form-group"><label>Order ID (Referencia)</label><input type="number" id="orderId" required></div>
                    <div class="form-group"><label>Monto</label><input type="number" step="0.01" id="amount" required></div>
                    <div class="form-group"><label>Banco Destino</label><input type="text" id="bank" required></div>
                    <button type="submit" class="btn-primary" id="btnProcesar">Procesar Operación</button>
                </form>
                <div id="paymentStatus" class="status-box">
                    <div id="statusLoader" class="loader"></div>
                    <h4 id="statusTitle" style="margin: 0 0 0.5rem 0;">Esperando Tarjeta...</h4>
                    <p id="statusMsg" style="margin:0; font-size: 0.9rem;">Por favor acerque la tarjeta al lector RFID.</p>
                </div>
            </div>
        </div>
        <!-- VISTA: HISTORIAL -->
        <div id="historial" class="view">
            <div class="card" style="max-width: 900px;">
                <div class="top-bar">
                    <h3 style="margin:0;">Historial de Operaciones</h3>
                    <div class="search-box">
                        <input type="text" id="searchInput" placeholder="Buscar ID...">
                        <button class="btn-small" onclick="searchOp()">Buscar</button>
                        <button class="btn-small" onclick="loadHistory()">🔄 Refrescar</button>
                    </div>
                </div>
                <div class="table-container">
                    <table id="historyTable">
                        <thead><tr><th>Operación ID</th><th>Monto</th><th>Estado</th><th>Fuente</th><th>Mensaje</th></tr></thead>
                        <tbody></tbody>
                    </table>
                </div>
            </div>
        </div>
        <!-- VISTA: LECTOR -->
        <div id="lector" class="view">
            <div class="card" style="text-align: center;">
                <h3 style="margin-top:0;">Lector Libre RFID</h3>
                <p style="color: var(--text-muted); margin-bottom: 2rem;">Presiona el botón y acerca una tarjeta para obtener su código único.</p>
                <div id="readerUI" style="display:none; margin-bottom: 2rem;">
                    <div class="loader" style="width: 40px; height: 40px; border-width: 4px;"></div>
                    <h4 style="margin: 1rem 0 0 0; color: var(--warning);">Lector Activo...</h4>
                </div>
                <div id="readerResult" style="display:none; margin-bottom: 2rem;">
                    <h4 style="margin: 0 0 0.5rem 0; color: var(--success);">¡Tarjeta Detectada!</h4>
                    <div style="font-family: monospace; font-size: 2rem; letter-spacing: 2px; padding: 1rem; background: rgba(0,0,0,0.3); border-radius: 10px; border: 1px solid var(--glass-border);" id="readerUidText"></div>
                </div>
                <button class="btn-primary" id="btnActivarLector" onclick="activateReader()">Activar Lector</button>
            </div>
        </div>
        <!-- VISTA: CONFIG -->
        <div id="config" class="view">
            <div class="card">
                <h3 style="margin-top:0;">Configuración del Sistema</h3>
                <form id="configForm">
                    <div class="form-group"><label>URL API Bancaria</label><input type="text" id="apiUrl" name="apiUrl"></div>
                    <div class="form-group"><label>API Key</label><input type="text" id="apiKey" name="apiKey"></div>
                    <div class="form-group"><label>URL WebSocket (Backend)</label><input type="text" id="backendUrl" name="backendUrl" placeholder="Dejar vacío para desactivar WS"></div>
                    <button type="submit" class="btn-primary">Guardar Cambios</button>
                </form>
                <button class="btn-danger" onclick="resetWifi()">Olvidar WiFi y Reiniciar</button>
            </div>
        </div>
    </div>
    <script>
        let pollInterval = null;
        let readerInterval = null;

        function switchView(viewId) {
            document.querySelectorAll('.view').forEach(v => v.classList.remove('active'));
            document.querySelectorAll('.nav-btn').forEach(b => b.classList.remove('active'));
            document.getElementById(viewId).classList.add('active');
            event.target.classList.add('active');
            
            if (viewId === 'historial') loadHistory();
            if (viewId === 'config') loadConfig();
            
            if (viewId !== 'lector' && readerInterval) {
                clearInterval(readerInterval);
                document.getElementById('btnActivarLector').disabled = false;
                document.getElementById('btnActivarLector').innerText = "Activar Lector";
                document.getElementById('readerUI').style.display = 'none';
            }
        }
        
        // --- LECTOR LIBRE ---
        async function activateReader() {
            const btn = document.getElementById('btnActivarLector');
            const ui = document.getElementById('readerUI');
            const res = document.getElementById('readerResult');
            
            btn.disabled = true; btn.innerText = "Esperando Tarjeta...";
            ui.style.display = 'block'; res.style.display = 'none';
            
            if(readerInterval) clearInterval(readerInterval);
            
            readerInterval = setInterval(async () => {
                try {
                    const r = await fetch('/api/read-card');
                    if(r.status === 200) {
                        const data = await r.json();
                        clearInterval(readerInterval);
                        ui.style.display = 'none'; res.style.display = 'block';
                        document.getElementById('readerUidText').innerText = data.uid;
                        btn.disabled = false; btn.innerText = "Leer Otra Tarjeta";
                    }
                } catch(e) {}
            }, 1000);
        }

        // --- PAGOS ---
        document.getElementById('payForm').addEventListener('submit', async (e) => {
            e.preventDefault();
            const btn = document.getElementById('btnProcesar');
            btn.disabled = true; btn.innerText = "Enviando...";
            const payload = { orderId: parseInt(document.getElementById('orderId').value), amount: parseFloat(document.getElementById('amount').value), destination: { bank: document.getElementById('bank').value } };
            try {
                const res = await fetch('/payments', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify(payload) });
                if(res.ok) { const data = await res.json(); document.getElementById('payForm').reset(); startPolling(data.operationId); } else { alert("Error enviando petición."); }
            } catch(e) { alert("Error de red."); }
            btn.disabled = false; btn.innerText = "Procesar Operación";
        });
        function startPolling(opId) {
            const sb = document.getElementById('paymentStatus');
            sb.className = 'status-box EN_COLA';
            document.getElementById('statusTitle').innerText = 'Esperando Tarjeta... (Ref: ' + opId + ')';
            document.getElementById('statusMsg').innerText = 'Por favor acerque la tarjeta al lector RFID.';
            document.getElementById('statusLoader').style.display = 'block';
            if(pollInterval) clearInterval(pollInterval);
            pollInterval = setInterval(async () => {
                try {
                    const r = await fetch('/payments/' + opId);
                    if(r.ok) {
                        const d = await r.json(); updateStatusBox(d.state, d.message);
                        if(d.state === 'COMPLETADO' || d.state === 'ERROR') { clearInterval(pollInterval); document.getElementById('statusLoader').style.display = 'none'; }
                    }
                } catch(e) {}
            }, 2000);
        }
        function updateStatusBox(state, msg) {
            const sb = document.getElementById('paymentStatus');
            sb.className = 'status-box ' + state;
            if(state === 'PROCESANDO') {
                if(msg && msg.includes('acerque')) {
                    document.getElementById('statusTitle').innerText = 'Esperando Tarjeta...';
                } else {
                    document.getElementById('statusTitle').innerText = 'Autorizando Pago...';
                }
            }
            else if(state === 'COMPLETADO') document.getElementById('statusTitle').innerText = '¡Pago Exitoso!';
            else if(state === 'ERROR') document.getElementById('statusTitle').innerText = 'Error en el Pago';
            document.getElementById('statusMsg').innerText = msg;
        }
        // --- HISTORIAL ---
        async function loadHistory() {
            try { const r = await fetch('/payments'); renderTable(await r.json()); } catch(e) {}
        }
        async function searchOp() {
            const term = document.getElementById('searchInput').value.trim();
            if(!term) return loadHistory();
            try { const r = await fetch('/payments/' + term); if(r.ok) { renderTable([await r.json()]); } else { renderTable([]); } } catch(e) {}
        }
        function renderTable(data) {
            const tb = document.querySelector('#historyTable tbody');
            tb.innerHTML = '';
            if(data.length === 0) { tb.innerHTML = '<tr><td colspan="5" style="text-align:center;">No hay resultados</td></tr>'; return; }
            data.forEach(item => { tb.innerHTML += `<tr><td>${item.operationId}</td><td>$${item.amount.toFixed(2)}</td><td><span class="badge ${item.state}">${item.state}</span></td><td>${item.source || '-'}</td><td>${item.message || '-'}</td></tr>`; });
        }
        // --- CONFIG ---
        async function loadConfig() {
            try {
                const r = await fetch('/api/config');
                if(r.ok) { const c = await r.json(); document.getElementById('apiUrl').value = c.apiUrl || ''; document.getElementById('apiKey').value = c.apiKey || ''; document.getElementById('backendUrl').value = c.backendUrl || ''; }
            } catch(e) {}
        }
        document.getElementById('configForm').addEventListener('submit', async (e) => {
            e.preventDefault();
            const btn = e.target.querySelector('button[type="submit"]'); btn.innerText = "Guardando...";
            const fd = new URLSearchParams(new FormData(e.target));
            try {
                const r = await fetch('/config', { method: 'POST', headers: { 'Content-Type': 'application/x-www-form-urlencoded' }, body: fd.toString() });
                if(r.ok) { btn.innerText = "¡Guardado!"; setTimeout(() => btn.innerText="Guardar Cambios", 2000); }
            } catch(e) { alert("Error guardando config"); }
        });
        async function resetWifi() {
            if(confirm('¿Estás seguro que deseas olvidar la red WiFi? El equipo se reiniciará.')) {
                await fetch('/reset-wifi', {method: 'POST'});
                alert("El equipo se está reiniciando. Conéctate a 'ESP32-POS-Config'.");
            }
        }
    </script>
</body>
</html>
)rawliteral";

CommsHandler::CommsHandler() : server(80) {
    globalComms = this;
    wsEnabled = false;
}

void CommsHandler::init() {
    paymentBuffer.init();
    orderQueue = xQueueCreate(10, sizeof(QueueItem));

    if (!MDNS.begin("esp32-pos")) {
        Serial.println("Error configurando mDNS!");
    } else {
        Serial.println("mDNS iniciado. URL: http://esp32-pos.local");
    }

    setupHttpServer();
    setupWebSocket();
}

void CommsHandler::setupHttpServer() {
    server.on("/", HTTP_GET, [this]() { this->handleHttpRoot(); });
    server.on("/api/config", HTTP_GET, [this]() { this->handleHttpGetConfig(); });
    server.on("/api/read-card", HTTP_GET, [this]() { this->handleHttpReadCard(); });
    server.on("/config", HTTP_POST, [this]() { this->handleHttpConfig(); });
    server.on("/reset-wifi", HTTP_POST, [this]() { this->handleHttpResetWiFi(); });
    
    server.on("/payments", HTTP_POST, [this]() { this->handleHttpPaymentsPost(); });
    server.on("/payments", HTTP_GET, [this]() { this->handleHttpPaymentsGetList(); });
    
    server.onNotFound([this]() {
        if (server.method() == HTTP_GET && server.uri().startsWith("/payments/")) {
            this->handleHttpPaymentsGetDetail();
        } else {
            this->server.send(404, "text/plain", "No encontrado");
        }
    });

    server.begin();
    Serial.println("Servidor HTTP local iniciado");
}

void CommsHandler::handleHttpRoot() {
    // Sirve el HTML desde la memoria Flash usando send_P
    server.send_P(200, "text/html", dashboardHtml);
}

void CommsHandler::handleHttpGetConfig() {
    JsonDocument doc;
    doc["apiUrl"] = apiUrl;
    doc["apiKey"] = apiKey;
    doc["backendUrl"] = backendUrl;
    String out;
    serializeJson(doc, out);
    server.send(200, "application/json", out);
}

void CommsHandler::handleHttpReadCard() {
    String uid = POSController::tryReadCard();
    if (uid.length() > 0) {
        server.send(200, "application/json", "{\"uid\":\"" + uid + "\"}");
    } else {
        server.send(204, "application/json", "{}");
    }
}

void CommsHandler::handleHttpConfig() {
    if (server.hasArg("apiUrl")) {
        apiUrl = server.arg("apiUrl");
    }
    if (server.hasArg("apiKey")) {
        apiKey = server.arg("apiKey");
    }
    if (server.hasArg("backendUrl")) {
        backendUrl = server.arg("backendUrl");
    }
    
    networkManager.saveConfig();
    
    // Restart WS logic if changed
    setupWebSocket();
    
    server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void CommsHandler::handleHttpResetWiFi() {
    server.send(200, "application/json", "{\"status\":\"ok\"}");
    delay(1000);
    networkManager.resetWiFi();
    ESP.restart();
}

void CommsHandler::handleHttpPaymentsPost() {
    if (server.hasArg("plain") == false) {
        server.send(400, "text/plain", "Falta el cuerpo JSON");
        return;
    }

    String body = server.arg("plain");
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, body);

    if (error) {
        server.send(400, "text/plain", "JSON Invalido");
        return;
    }

    uint32_t orderId = doc["orderId"] | 0;
    float amount = doc["amount"] | 0.0f;
    String rawPayload = body; 
    String opIdStr = "OP-" + String(millis()) + "-" + String(orderId);
    
    OperacionPago* op = paymentBuffer.addOperation(opIdStr.c_str(), orderId, amount, rawPayload.c_str(), "HTTP");
    
    if (op != nullptr) {
        QueueItem item;
        strlcpy(item.operationId, op->operationId, sizeof(item.operationId));
        
        if (xQueueSend(orderQueue, &item, (TickType_t)0) == pdPASS) {
            String response = "{\"operationId\":\"" + opIdStr + "\", \"status\":\"accepted\"}";
            server.send(202, "application/json", response);
        } else {
            paymentBuffer.updateState(opIdStr.c_str(), ERROR, "Cola llena");
            server.send(503, "application/json", "{\"status\":\"error\", \"message\":\"Cola llena\"}");
        }
    } else {
        server.send(500, "application/json", "{\"status\":\"error\", \"message\":\"Error de Buffer\"}");
    }
}

void CommsHandler::handleHttpPaymentsGetList() {
    String jsonList = paymentBuffer.getAllOperationsJson();
    server.send(200, "application/json", jsonList);
}

void CommsHandler::handleHttpPaymentsGetDetail() {
    String uri = server.uri();
    String opId = uri.substring(10); 

    OperacionPago* op = paymentBuffer.getOperationById(opId.c_str());
    if (op != nullptr) {
        JsonDocument doc;
        doc["operationId"] = op->operationId;
        doc["orderId"] = op->orderId;
        doc["amount"] = op->amount;
        
        String estadoStr;
        switch(op->estado) {
            case EN_COLA: estadoStr = "EN_COLA"; break;
            case PROCESANDO: estadoStr = "PROCESANDO"; break;
            case COMPLETADO: estadoStr = "COMPLETADO"; break;
            case ERROR: estadoStr = "ERROR"; break;
        }
        doc["state"] = estadoStr;
        doc["source"] = op->source;
        doc["message"] = op->responseMessage;
        
        String out;
        serializeJson(doc, out);
        server.send(200, "application/json", out);
    } else {
        server.send(404, "application/json", "{\"error\":\"Operacion no encontrada\"}");
    }
}

void CommsHandler::setupWebSocket() {
    if (wsEnabled) {
        webSocket.disconnect();
    }

    if (backendUrl.length() == 0) {
        Serial.println("[WS] backendUrl vacío. WebSocket deshabilitado.");
        wsEnabled = false;
        return;
    }
    
    wsEnabled = true;
    String protocol = "ws";
    int port = 80;
    String host = "";
    String path = "/";

    if (backendUrl.startsWith("wss://")) {
        protocol = "wss";
        port = 443;
        int pathIdx = backendUrl.indexOf("/", 6);
        if (pathIdx > 0) {
            host = backendUrl.substring(6, pathIdx);
            path = backendUrl.substring(pathIdx);
        } else {
            host = backendUrl.substring(6);
        }
    } else if (backendUrl.startsWith("ws://")) {
        protocol = "ws";
        port = 80;
        int pathIdx = backendUrl.indexOf("/", 5);
        if (pathIdx > 0) {
            host = backendUrl.substring(5, pathIdx);
            path = backendUrl.substring(pathIdx);
        } else {
            host = backendUrl.substring(5);
        }
    } else {
        host = backendUrl;
    }

    if (protocol == "wss") {
        webSocket.beginSSL(host.c_str(), port, path.c_str());
    } else {
        webSocket.begin(host.c_str(), port, path.c_str());
    }
    
    webSocket.onEvent(webSocketEvent);
    webSocket.setReconnectInterval(5000);
    Serial.println("[WS] WebSocket inicializado hacia: " + backendUrl);
}

void CommsHandler::webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
    switch(type) {
        case WStype_DISCONNECTED:
            Serial.println("[WS] Desconectado!");
            break;
        case WStype_CONNECTED:
            Serial.println("[WS] Conectado al backend");
            globalComms->webSocket.sendTXT("{\"event\":\"device_ready\"}");
            break;
        case WStype_TEXT:
        {
            String body = String((char*)payload, length);
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, body);

            if (!error && !doc["orderId"].isNull()) {
                uint32_t orderId = doc["orderId"] | 0;
                float amount = doc["amount"] | 0.0f;
                String opIdStr = "OP-" + String(millis()) + "-" + String(orderId);
                
                OperacionPago* op = paymentBuffer.addOperation(opIdStr.c_str(), orderId, amount, body.c_str(), "WSS");
                
                if (op != nullptr) {
                    QueueItem item;
                    strlcpy(item.operationId, op->operationId, sizeof(item.operationId));
                    if (xQueueSend(orderQueue, &item, (TickType_t)0) == pdPASS) {
                        String resp = "{\"operationId\":\"" + opIdStr + "\", \"status\":\"accepted\"}";
                        globalComms->webSocket.sendTXT(resp.c_str());
                    } else {
                        paymentBuffer.updateState(opIdStr.c_str(), ERROR, "Cola llena");
                        String err = "{\"operationId\":\"" + opIdStr + "\", \"status\":\"error\"}";
                        globalComms->webSocket.sendTXT(err.c_str());
                    }
                }
            }
            break;
        }
        default:
            break;
    }
}

void CommsHandler::loop() {
    server.handleClient();
    if (wsEnabled) {
        webSocket.loop();
    }
}

void CommsHandler::sendWebSocketMessage(const String& msg) {
    if (wsEnabled) {
        webSocket.sendTXT(msg.c_str());
    }
}
