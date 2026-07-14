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
    <title>Cineflix POS Terminal</title>
    <style>
        :root {
            --bg-main: #0f111a; --bg-header: #161925; --bg-sidebar: #1a1d27;
            --text-main: #f8f8f2; --text-muted: #8a95ab;
            --c-green: #a6e22e; --c-yellow: #e6db74; --c-blue: #66d9ef; --c-red: #f92672;
            --font-mono: 'Consolas', 'Courier New', monospace;
            --font-sans: system-ui, -apple-system, sans-serif;
            --border-c: #2a2e3d;
        }
        * { box-sizing: border-box; margin: 0; padding: 0; }        /* Layout base */
        body { margin: 0; background: var(--bg-main); color: var(--text-main); font-family: var(--font-sans); height: 100vh; overflow: hidden; }
        .layout { display: flex; height: 100%; width: 100%; }
        .main-wrapper { flex: 1; display: flex; flex-direction: column; overflow: hidden; margin-right: 320px; transition: margin-right 0.3s ease; }
        .layout.sidebar-closed .main-wrapper { margin-right: 0; }
        
        /* Header */
        .header { background: var(--bg-header); min-height: 48px; display: flex; flex-wrap: wrap; align-items: center; padding: 0.5rem 1.5rem; border-bottom: 1px solid var(--border-c); justify-content: space-between; gap: 0.5rem; box-sizing: border-box; position: relative; z-index: 1002; }
        .header-left { display: flex; align-items: center; gap: 0.75rem; font-family: var(--font-sans); font-size: 0.85rem; font-weight: 600; color: #a0aabf; }
        .dot { width: 8px; height: 8px; background: var(--c-green); border-radius: 50%; box-shadow: 0 0 8px var(--c-green); }
        .header-right { display: flex; align-items: center; gap: 1rem; flex-wrap: wrap; }
        .nav-link { color: var(--text-muted); cursor: pointer; font-size: 0.85rem; transition: 0.2s; font-family: var(--font-sans); font-weight: 500; }
        .nav-link:hover, .nav-link.active { color: var(--c-blue); }

        .main { flex: 1; padding: 2rem; overflow-y: auto; }
        
        /* Sidebar */
        .sidebar { position: fixed; right: 0; top: 0; height: 100vh; width: 320px; background: var(--bg-sidebar); border-left: 1px solid var(--border-c); display: flex; flex-direction: column; z-index: 1000; transition: transform 0.3s ease; }
        .layout.sidebar-closed .sidebar { transform: translateX(100%); }
        .sidebar-header { padding: 1rem 1.25rem; font-family: var(--font-sans); font-size: 0.7rem; font-weight: 700; color: var(--text-muted); letter-spacing: 1px; border-bottom: 1px solid var(--border-c); background: var(--bg-header); min-height: 48px; display: flex; align-items: center; box-sizing: border-box; }
        .sidebar-content { flex: 1; overflow-y: auto; padding: 1rem; display: flex; flex-direction: column; gap: 0.75rem; }

        /* Terminal Texts */
        .log-line { margin-bottom: 0.5rem; font-size: 0.9rem; line-height: 1.5; }
        .c-green { color: var(--c-green); } .c-yellow { color: var(--c-yellow); }
        .c-blue { color: var(--c-blue); } .c-red { color: var(--c-red); }

        /* Forms */
        .form-group { margin-bottom: 1.2rem; }
        label { display: block; color: var(--c-blue); margin-bottom: 0.5rem; font-size: 0.9rem; }
        input { width: 100%; max-width: 400px; background: var(--bg-header); border: 1px solid var(--border-c); color: var(--c-yellow); padding: 0.75rem; font-family: var(--font-mono); font-size: 0.95rem; border-radius: 4px; outline: none; }
        input:focus { border-color: var(--c-blue); }
        button { background: var(--border-c); color: var(--text-main); border: 1px solid #3b4054; padding: 0.75rem 1.5rem; font-family: var(--font-mono); font-size: 0.9rem; cursor: pointer; border-radius: 4px; transition: 0.2s; }
        button:hover { background: #3b4054; color: var(--c-blue); }
        button.primary { background: rgba(102, 217, 239, 0.1); border-color: var(--c-blue); color: var(--c-blue); }
        button.primary:hover { background: rgba(102, 217, 239, 0.2); }

        /* Sidebar Cards (Events) */
        .event-card { background: var(--bg-main); border: 1px solid var(--border-c); border-radius: 6px; padding: 0.75rem; font-size: 0.8rem; display: flex; flex-direction: column; gap: 0.5rem; cursor: pointer; transition: background 0.2s; }
        .event-card:hover { background: rgba(255,255,255,0.05); }
        .event-header { display: flex; justify-content: space-between; align-items: center; }
        .badge { background: rgba(102, 217, 239, 0.15); color: var(--c-blue); padding: 2px 6px; border-radius: 4px; font-size: 0.65rem; font-family: var(--font-sans); font-weight: bold; border: 1px solid rgba(102, 217, 239, 0.3); }
        .badge.COMPLETADO { background: rgba(166, 226, 46, 0.15); color: var(--c-green); border-color: rgba(166, 226, 46, 0.3); }
        .badge.ERROR { background: rgba(249, 38, 114, 0.15); color: var(--c-red); border-color: rgba(249, 38, 114, 0.3); }
        .badge.EN_COLA { background: rgba(230, 219, 116, 0.15); color: var(--c-yellow); border-color: rgba(230, 219, 116, 0.3); }
        .time { color: var(--text-muted); font-size: 0.7rem; font-family: var(--font-sans); }
        .event-body { color: #a0aabf; line-height: 1.4; word-break: break-all; }

        .modal-overlay { position: fixed; top: 0; left: 0; width: 100%; height: 100%; background: rgba(0,0,0,0.7); z-index: 2000; display: none; align-items: center; justify-content: center; backdrop-filter: blur(2px); }
        .modal-overlay.active { display: flex; animation: fadeIn 0.2s; }
        .modal-content { background: var(--bg-main); border: 1px solid var(--border-c); border-radius: 6px; width: 90%; max-width: 400px; padding: 1.5rem; box-shadow: 0 10px 30px rgba(0,0,0,0.5); }
        .modal-header { font-size: 1.1rem; color: var(--c-blue); margin-bottom: 1rem; display: flex; justify-content: space-between; align-items: center; font-weight: bold; font-family: var(--font-sans); }
        .modal-close { cursor: pointer; color: var(--text-muted); font-size: 1.2rem; }
        .modal-close:hover { color: var(--c-red); }
        .modal-body { color: var(--text-main); font-size: 0.9rem; line-height: 1.6; }
        .modal-body strong { color: #a0aabf; }

        .view { display: none; }
        .view.active { display: block; animation: fadeIn 0.2s ease-out; }
        @keyframes fadeIn { from { opacity: 0; transform: translateX(-5px); } to { opacity: 1; transform: translateX(0); } }
        
        .split-view { display: flex; gap: 2rem; align-items: flex-start; }
        #payForm, #configForm { flex: 1; max-width: 400px; width: 100%; }
        .status-frame { flex: 1; min-width: 300px; border: 1px solid var(--border-c); border-radius: 6px; padding: 1.25rem; background: rgba(0,0,0,0.2); box-shadow: inset 0 0 10px rgba(0,0,0,0.5); }
        
        .title-short { display: none; }
        @media (max-width: 900px) {
            .main-wrapper { margin-right: 0; }
            .sidebar { width: 100%; box-shadow: -5px 0 20px rgba(0,0,0,0.5); }
        }
        @media (max-width: 768px) {
            .header { padding: 0.5rem 1rem; }
            .header-left { font-size: 0.8rem; }
            .header-right { gap: 0.6rem; }
            .nav-link { font-size: 0.75rem; }
            .main { padding: 1rem; }
            .split-view { flex-direction: column; gap: 1rem; }
            #payForm, #configForm { max-width: 100%; }
            .status-frame { width: 100%; min-width: 0; }
        }
        @media (max-width: 480px) {
            .title-long { display: none; }
            .title-short { display: inline; }
            .header-right { gap: 0.4rem; }
            .nav-link { font-size: 0.7rem; }
        }
    </style>
</head>
<body>
    <div class="layout">
        <div class="main-wrapper">
            <div class="header">
                <div class="header-left">
                    <div class="dot"></div> 
                    <span class="title-long">Cineflix POS Terminal</span>
                    <span class="title-short">POS</span>
                </div>
                <div class="header-right">
                    <div class="nav-link active" onclick="switchView('pagos', this)">COBRO</div>
                    <div class="nav-link" onclick="switchView('lector', this)">LECTOR</div>
                    <div class="nav-link" onclick="switchView('config', this)">CONFIG</div>
                    <div class="nav-link" id="btnToggleEvents" style="border-left: 1px solid var(--border-c); padding-left: 0.75rem; display: flex; align-items: center;" onclick="toggleSidebar()">
                        <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="3" y="3" width="18" height="18" rx="2" ry="2"></rect><line x1="15" y1="3" x2="15" y2="21"></line></svg>
                    </div>
                </div>
            </div>
            
            <div class="main">
                <div id="pagos" class="view active">
                    <div class="log-line"><span class="c-blue">root@pos</span>:<span class="c-green">~/cobro</span>$ init_transaction</div>
                    <br>
                    <div class="split-view">
                        <form id="payForm">
                            <div class="form-group"><label>>_ Order ID</label><input type="number" id="orderId" required></div>
                            <div class="form-group"><label>>_ Monto ($)</label><input type="number" step="0.01" id="amount" required></div>
                            <div class="form-group"><label>>_ Documento Destino</label><input type="text" id="destDoc" required placeholder="V-12345678"></div>
                            <div class="form-group"><label>>_ Cuenta Destino</label><input type="text" id="destAcc" required placeholder="01020000000000000000"></div>
                            <div style="display:flex; gap:0.5rem;">
                                <button type="submit" class="primary" id="btnProcesar">>_ EXECUTE</button>
                                <button type="reset" class="secondary" style="background:var(--bg-main);">>_ CLEAR</button>
                            </div>
                        </form>
                        
                        <div id="paymentStatus" class="status-frame" style="display:none;">
                            <div class="log-line" style="color:var(--text-muted); margin-bottom:1rem; font-weight:bold;">>_ STATUS LOG:</div>
                            <div id="termStatus" class="log-line"></div>
                        </div>
                    </div>
                </div>
                
                <div id="lector" class="view">
                    <div class="log-line"><span class="c-blue">root@pos</span>:<span class="c-green">~/lector</span>$ read_rfid_card</div>
                    <br>
                    <button type="button" class="primary" id="btnActivarLector" onclick="activateReader()">>_ START SCAN</button>
                    <br><br>
                    <div id="readerUI" style="display:none;" class="log-line c-yellow">[~] Scanning...</div>
                    <div id="readerResult" style="display:none;" class="log-line">
                        <span class="c-green">[✓] Card Detected: </span> <span class="c-blue" id="readerUidText"></span>
                    </div>
                </div>
                
                <div id="config" class="view">
                    <div class="log-line"><span class="c-blue">root@pos</span>:<span class="c-green">~/config</span>$ edit_network</div>
                    <br>
                    
                    <div style="background:var(--bg-header); padding:1rem; border:1px solid var(--border-c); border-radius:4px; margin-bottom:1.5rem; display:flex; align-items:center; justify-content:space-between; flex-wrap:wrap; gap:1rem;">
                        <div style="display:flex; align-items:center; gap:10px;">
                            <div id="wsIndicator" style="width:12px; height:12px; border-radius:50%; background:var(--c-red); box-shadow:0 0 8px var(--c-red);"></div>
                            <span id="wsStatusText" style="font-weight:bold; font-size:0.9rem;">Desconectado</span>
                        </div>
                        <div style="display:flex; gap:0.5rem;">
                            <button type="button" class="primary" onclick="connectWs()">>_ CONNECT WS</button>
                            <button type="button" class="secondary" style="background:var(--bg-main);" onclick="disconnectWs()">>_ DISCONNECT</button>
                        </div>
                    </div>
                    
                    <form id="configForm">
                        <div class="form-group"><label>>_ URL API</label><input type="url" id="apiUrl" name="apiUrl"></div>
                        <div class="form-group"><label>>_ API Key</label><input type="password" id="apiKey" name="apiKey"></div>
                        <div class="form-group"><label>>_ URL WebSocket</label><input type="text" id="backendUrl" name="backendUrl"></div>
                        <div class="form-group"><label>>_ POS API Key (WS)</label><input type="password" id="posApiKey" name="posApiKey"></div>
                        <div class="form-group"><label>>_ SSID Wifi</label><input type="text" id="ssid"></div>
                        <div class="form-group"><label>>_ Password Wifi</label><input type="password" id="password"></div>
                        <button type="submit" class="primary">>_ SAVE</button>
                    </form>
                    <br><br>
                    <div class="log-line"><span class="c-blue">root@pos</span>:<span class="c-green">~/config</span>$ reset_network</div>
                    <br>
                    <button type="button" style="color:var(--c-red); border-color:rgba(249,38,114,0.4);" onclick="resetWifi()">>_ PURGE_WIFI</button>
                </div>
            </div>
        <!-- REGISTRO DE EVENTOS (Sidebar) -->
        <div class="sidebar">
            <div class="sidebar-header">REGISTRO DE EVENTOS</div>
            <div class="sidebar-content" id="historyList">
                <div class="log-line c-muted" style="text-align:center; margin-top:2rem;">[ Terminal iniciada. Esperando eventos... ]</div>
            </div>
        </div>
    </div>
    
    <div class="modal-overlay" id="eventModal" onclick="if(event.target===this)closeModal()">
        <div class="modal-content">
            <div class="modal-header">
                <span>Detalle de Transacción</span>
                <span class="modal-close" onclick="closeModal()">✕</span>
            </div>
            <div class="modal-body" id="modalBody"></div>
        </div>
    </div>
    
    <script>
        let pollInterval = null;
        let readerInterval = null;

        function switchView(viewId, btnEl) {
            document.querySelectorAll('.view').forEach(v => v.classList.remove('active'));
            // Remove active class from view links, but not the toggle button
            document.querySelectorAll('.header-right .nav-link').forEach(b => {
                if(b.id !== 'btnToggleEvents') b.classList.remove('active');
            });
            document.getElementById(viewId).classList.add('active');
            btnEl.classList.add('active');
            
            if (viewId === 'config') loadConfig();
            
            if (viewId === 'lector') {
                document.getElementById('readerUI').style.display = 'none';
                document.getElementById('readerResult').style.display = 'none';
                document.getElementById('readerUidText').innerText = '';
                document.getElementById('btnActivarLector').disabled = false;
                document.getElementById('btnActivarLector').innerText = ">_ START SCAN";
                if (readerInterval) { clearInterval(readerInterval); readerInterval = null; }
            } else if (readerInterval) {
                clearInterval(readerInterval);
                readerInterval = null;
                document.getElementById('btnActivarLector').disabled = false;
                document.getElementById('btnActivarLector').innerText = ">_ START SCAN";
                document.getElementById('readerUI').style.display = 'none';
            }
            
            if (viewId === 'pagos') {
                if (!pollInterval) {
                    document.getElementById('payForm').reset();
                    document.getElementById('paymentStatus').style.display = 'none';
                    document.getElementById('termStatus').innerHTML = '';
                }
            }
        }
        
        function toggleSidebar() {
            document.querySelector('.layout').classList.toggle('sidebar-closed');
        }
        
        function updateSidebarPadding() {
            if (window.innerWidth <= 900) {
                const h = document.querySelector('.header').offsetHeight;
                document.querySelector('.sidebar').style.paddingTop = h + 'px';
            } else {
                document.querySelector('.sidebar').style.paddingTop = '0px';
            }
        }
        window.addEventListener('resize', updateSidebarPadding);
        window.addEventListener('DOMContentLoaded', updateSidebarPadding);
        
        async function activateReader() {
            const btn = document.getElementById('btnActivarLector');
            const ui = document.getElementById('readerUI');
            const res = document.getElementById('readerResult');
            
            btn.disabled = true; btn.innerText = "[~] AWAITING...";
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
                        btn.disabled = false; btn.innerText = ">_ SCAN AGAIN";
                    }
                } catch(e) {}
            }, 1000);
        }

        document.getElementById('payForm').addEventListener('submit', async (e) => {
            e.preventDefault();
            const btn = document.getElementById('btnProcesar');
            btn.disabled = true; btn.innerText = "[~] EXECUTING...";
            document.getElementById('paymentStatus').style.display = 'block';
            document.getElementById('termStatus').innerHTML = '';
            
            const payload = { orderId: parseInt(document.getElementById('orderId').value), amount: parseFloat(document.getElementById('amount').value), destinationDocument: document.getElementById('destDoc').value, destinationAccount: document.getElementById('destAcc').value };
            try {
                logTerm('c-blue', '[*]', 'Enviando payload al backend...');
                const res = await fetch('/payments', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify(payload) });
                if(res.ok) { 
                    const data = await res.json(); 
                    logTerm('c-yellow', '[~]', 'Operacion aceptada. ID: ' + data.operationId);
                    startPolling(data.operationId); 
                    loadHistory();
                } else { 
                    logTerm('c-red', '[!]', 'Error de API.'); 
                }
            } catch(e) { logTerm('c-red', '[!]', 'Error de red.'); }
            btn.disabled = false; btn.innerText = ">_ EXECUTE";
        });
        
        function logTerm(color, icon, text) {
            document.getElementById('termStatus').innerHTML += `<div><span class="${color}">${icon}</span> ${text}</div>`;
        }

        let lastMsg = '';
        let timerCountdown = null;
        function startPolling(opId) {
            logTerm('c-yellow', '[~]', 'En cola. Esperando tarjeta...');
            lastMsg = 'En cola. Esperando tarjeta...';
            if(pollInterval) clearInterval(pollInterval);
            if(timerCountdown) clearInterval(timerCountdown);
            
            pollInterval = setInterval(async () => {
                try {
                    const r = await fetch('/payments/' + opId);
                    if(r.ok) {
                        const d = await r.json(); 
                        let col = 'c-blue', ico = '[*]';
                        if(d.state === 'PROCESANDO') { col = 'c-yellow'; ico = '[~]'; }
                        if(d.state === 'COMPLETADO') { col = 'c-green'; ico = '[✓]'; }
                        if(d.state === 'ERROR') { col = 'c-red'; ico = '[!]'; }
                        
                        let currentMsg = d.message || d.state;
                        if (currentMsg !== lastMsg) {
                            lastMsg = currentMsg;
                            
                            if (currentMsg === 'Por favor acerque la tarjeta al lector.') {
                                logTerm(col, ico, currentMsg + ' <span id="waitTimer" style="color:var(--c-red); font-weight:bold; margin-left:10px;">15</span>s');
                                let secs = 15;
                                if(timerCountdown) clearInterval(timerCountdown);
                                timerCountdown = setInterval(() => {
                                    secs--;
                                    let el = document.getElementById('waitTimer');
                                    if(el && secs >= 0) el.innerText = secs;
                                }, 1000);
                            } else {
                                if(timerCountdown) clearInterval(timerCountdown);
                                logTerm(col, ico, currentMsg);
                            }
                        }
                        
                        loadHistory();
                        if(d.state === 'COMPLETADO' || d.state === 'ERROR') { 
                            clearInterval(pollInterval);
                            pollInterval = null;
                            if(timerCountdown) clearInterval(timerCountdown);
                            if(d.state === 'COMPLETADO') document.getElementById('payForm').reset();
                        }
                    }
                } catch(e) {}
            }, 2000);
        }
        
        async function loadHistory() {
            try { const r = await fetch('/payments'); renderList(await r.json()); } catch(e) {}
        }
        
        function formatTime(ts) {
            if (!ts) return '-';
            return new Date(ts * 1000).toLocaleString();
        }
        
        function openModal(dataStr) {
            const item = JSON.parse(decodeURIComponent(dataStr));
            let payloadObj = {};
            try { payloadObj = JSON.parse(item.rawPayload || '{}'); } catch(e) {}
            
            let destAcc = payloadObj.destinationAccount || '-';
            if (typeof destAcc === 'object') {
                destAcc = destAcc.bank ? (destAcc.bank + " - " + destAcc.number) : JSON.stringify(destAcc);
            }
            
            let html = `
                <div style="margin-bottom:10px;"><strong>Orden Interna:</strong> ${item.orderId}</div>
                <div style="margin-bottom:10px;"><strong>ID de Operación:</strong> ${item.operationId}</div>
                <div style="margin-bottom:10px;"><strong>Cuenta Destino:</strong> ${destAcc}</div>
                <div style="margin-bottom:10px;"><strong>Documento Destino:</strong> ${payloadObj.destinationDocument || '-'}</div>
                <div style="margin-bottom:10px;"><strong>Origen:</strong> ${item.source}</div>
                <div style="margin-bottom:10px;"><strong>Monto:</strong> $${item.amount.toFixed(2)}</div>
                <div style="margin-bottom:10px;"><strong>Estado:</strong> <span class="badge ${item.state}">${item.state}</span></div>
                <div style="margin-bottom:10px;"><strong>Mensaje:</strong> ${item.message || '-'}</div>
                <hr style="border:none; border-top:1px solid var(--border-c); margin: 15px 0;">
                <div style="margin-bottom:5px;"><strong>Creada:</strong> ${formatTime(item.createdAt)}</div>
                <div style="margin-bottom:5px;"><strong>Actualizada:</strong> ${formatTime(item.updatedAt)}</div>
            `;
            if (item.source === 'WSS' && item.emittedAt) {
                html += `<div style="margin-bottom:5px;"><strong>Emitida WS:</strong> ${formatTime(item.emittedAt)}</div>`;
            }
            document.getElementById('modalBody').innerHTML = html;
            document.getElementById('eventModal').classList.add('active');
        }
        
        function closeModal() {
            document.getElementById('eventModal').classList.remove('active');
        }

        function renderList(data) {
            const hl = document.getElementById('historyList');
            if(data.length === 0) { hl.innerHTML = '<div class="log-line" style="color:var(--text-muted); text-align:center;">[ EMPTY LOG ]</div>'; return; }
            hl.innerHTML = '';
            
            // Invert array to show newest first
            [...data].reverse().forEach(item => { 
                let badgeClass = item.state;
                let text = `ID: ${item.operationId} | $${item.amount.toFixed(2)}<br><span style="color:var(--text-muted)">${item.message || '-'}</span>`;
                let safeData = encodeURIComponent(JSON.stringify(item)).replace(/'/g, "%27");
                let timeStr = formatTime(item.updatedAt || item.createdAt);
                if (timeStr === '-') timeStr = new Date().toLocaleTimeString();
                else timeStr = new Date((item.updatedAt || item.createdAt)*1000).toLocaleTimeString();
                
                hl.innerHTML += `
                <div class="event-card" onclick="openModal('${safeData}')">
                    <div class="event-header">
                        <div style="display:flex; gap:0.5rem; align-items:center;">
                            <span class="badge ${badgeClass}">${item.state}</span>
                            <span class="badge" style="background:transparent; border-color:var(--border-c); color:var(--text-muted);">${item.source}</span>
                        </div>
                        <span class="time">${timeStr}</span>
                    </div>
                    <div class="event-body">${text}</div>
                </div>`; 
            });
        }
        
        // Polling global del historial
        setInterval(loadHistory, 3000);
        window.addEventListener('DOMContentLoaded', loadHistory);
        
        async function loadConfig() {
            try {
                const r = await fetch('/api/config');
                if(r.ok) { const c = await r.json(); document.getElementById('apiUrl').value = c.apiUrl || ''; document.getElementById('apiKey').value = c.apiKey || ''; document.getElementById('backendUrl').value = c.backendUrl || ''; document.getElementById('posApiKey').value = c.posApiKey || ''; }
            } catch(e) {}
        }
        document.getElementById('configForm').addEventListener('submit', async (e) => {
            e.preventDefault();
            const btn = e.target.querySelector('button[type="submit"]'); btn.innerText = "[~] SAVING...";
            const fd = new URLSearchParams(new FormData(e.target));
            try {
                const r = await fetch('/config', { method: 'POST', headers: { 'Content-Type': 'application/x-www-form-urlencoded' }, body: fd.toString() });
                if(r.ok) { btn.innerText = "[✓] SAVED"; setTimeout(() => btn.innerText=">_ SAVE", 2000); }
            } catch(e) { alert("Error"); }
        });
        async function resetWifi() {
            if(confirm('¿Purge WiFi & Reboot?')) {
                await fetch('/reset-wifi', {method: 'POST'});
                alert("Rebooting...");
            }
        }
        
        loadConfig();
        setInterval(loadHistory, 3000); // Auto-refresh history panel

        async function connectWs() {
            document.getElementById('wsIndicator').style.background = 'var(--c-yellow)';
            document.getElementById('wsIndicator').style.boxShadow = '0 0 8px var(--c-yellow)';
            document.getElementById('wsStatusText').innerText = 'Conectando...';
            document.getElementById('wsStatusText').style.color = 'var(--c-yellow)';
            try { await fetch('/api/ws-connect', { method: 'POST' }); } catch(e) {}
            setTimeout(checkWsStatus, 2000);
        }

        async function disconnectWs() {
            document.getElementById('wsIndicator').style.background = 'var(--c-yellow)';
            document.getElementById('wsIndicator').style.boxShadow = '0 0 8px var(--c-yellow)';
            document.getElementById('wsStatusText').innerText = 'Desconectando...';
            document.getElementById('wsStatusText').style.color = 'var(--c-yellow)';
            try { await fetch('/api/ws-disconnect', { method: 'POST' }); } catch(e) {}
            setTimeout(checkWsStatus, 2000);
        }

        async function checkWsStatus() {
            try {
                const res = await fetch('/api/ws-status');
                if(res.ok) {
                    const data = await res.json();
                    const ind = document.getElementById('wsIndicator');
                    const txt = document.getElementById('wsStatusText');
                    if(data.connected) {
                        ind.style.background = 'var(--c-green)';
                        ind.style.boxShadow = '0 0 8px var(--c-green)';
                        txt.innerText = 'Conectado';
                        txt.style.color = 'var(--c-green)';
                    } else {
                        ind.style.background = 'var(--c-red)';
                        ind.style.boxShadow = '0 0 8px var(--c-red)';
                        txt.innerText = 'Desconectado';
                        txt.style.color = 'var(--c-red)';
                    }
                }
            } catch(e) {}
        }
        
        setInterval(checkWsStatus, 3000);
        checkWsStatus();

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

    if (!MDNS.begin("cineflix-pos")) {
        Serial.println("Error configurando mDNS!");
    } else {
        Serial.println("mDNS iniciado. URL: http://cineflix-pos.local");
    }

    setupHttpServer();
    setupWebSocket();
}

void CommsHandler::setupHttpServer() {
    server.on("/", HTTP_GET, [this]() { this->handleHttpRoot(); });
    server.on("/api/config", HTTP_GET, [this]() { this->handleHttpGetConfig(); });
    server.on("/api/read-card", HTTP_GET, [this]() { this->handleHttpReadCard(); });
    server.on("/config", HTTP_POST, [this]() { this->handleHttpConfig(); });
    server.on("/payments", HTTP_POST, std::bind(&CommsHandler::handleHttpPaymentsPost, this));
    server.on("/payments", HTTP_GET, std::bind(&CommsHandler::handleHttpPaymentsGetList, this));
    server.on(UriBraces("/payments/{}"), HTTP_GET, std::bind(&CommsHandler::handleHttpPaymentsGetDetail, this));

    server.on("/api/ws-connect", HTTP_POST, std::bind(&CommsHandler::handleHttpWsConnect, this));
    server.on("/api/ws-disconnect", HTTP_POST, std::bind(&CommsHandler::handleHttpWsDisconnect, this));
    server.on("/api/ws-status", HTTP_GET, std::bind(&CommsHandler::handleHttpWsStatus, this));

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
    doc["posApiKey"] = posApiKey;
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
    if (server.hasArg("posApiKey")) {
        posApiKey = server.arg("posApiKey");
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
        doc["rawPayload"] = op->jsonPayload;
        
        String out;
        serializeJson(doc, out);
        server.send(200, "application/json", out);
    } else {
        server.send(404, "application/json", "{\"error\":\"Operacion no encontrada\"}");
    }
}

void CommsHandler::handleHttpWsConnect() {
    setupWebSocket();
    server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void CommsHandler::handleHttpWsDisconnect() {
    wsEnabled = false;
    socketIO.disconnect();
    server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void CommsHandler::handleHttpWsStatus() {
    bool connected = socketIO.isConnected();
    String resp = "{\"connected\":" + String(connected ? "true" : "false") + "}";
    server.send(200, "application/json", resp);
}

void CommsHandler::setupWebSocket() {
    if (wsEnabled) {
        socketIO.disconnect();
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

    String cleanUrl = backendUrl;
    if (cleanUrl.startsWith("https://") || cleanUrl.startsWith("wss://")) {
        protocol = "wss";
        port = 443;
        cleanUrl = cleanUrl.substring(cleanUrl.indexOf("://") + 3);
    } else if (cleanUrl.startsWith("http://") || cleanUrl.startsWith("ws://")) {
        protocol = "ws";
        port = 80;
        cleanUrl = cleanUrl.substring(cleanUrl.indexOf("://") + 3);
    }

    int pathIdx = cleanUrl.indexOf("/");
    if (pathIdx > 0) {
        path = cleanUrl.substring(pathIdx);
        cleanUrl = cleanUrl.substring(0, pathIdx);
    }

    int portIdx = cleanUrl.indexOf(":");
    if (portIdx > 0) {
        port = cleanUrl.substring(portIdx + 1).toInt();
        host = cleanUrl.substring(0, portIdx);
    } else {
        host = cleanUrl;
    }

    if (protocol == "wss") {
        socketIO.beginSSL(host.c_str(), port, "/socket.io/?EIO=4&transport=websocket");
    } else {
        socketIO.begin(host.c_str(), port, "/socket.io/?EIO=4&transport=websocket");
    }
    
    socketIO.onEvent(socketIOEvent);
    socketIO.setReconnectInterval(5000);
    Serial.println("[WS] WebSocket inicializado hacia: " + backendUrl);
}

void CommsHandler::socketIOEvent(socketIOmessageType_t type, uint8_t * payload, size_t length) {
    switch(type) {
        case sIOtype_DISCONNECT:
            Serial.println("[SocketIO] Desconectado!");
            break;
        case sIOtype_CONNECT:
        {
            Serial.println("[SocketIO] TCP Conectado. Forzando handshake Socket.IO (40)...");
            globalComms->socketIO.send(sIOtype_CONNECT, "/");
            globalComms->pendingWsAuth = true;
            globalComms->wsAuthTimer = millis();
            break;
        }
        case sIOtype_EVENT:
        {
            String body = String((char*)payload, length);
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, body);

            // Socket.IO events are arrays: ["event_name", {payload}]
            if (!error && doc.is<JsonArray>() && doc.size() > 1) {
                String eventName = doc[0].as<String>();
                JsonObject eventData = doc[1].as<JsonObject>();
                
                if (eventName == "pos:process_payment" && !eventData["orderId"].isNull()) {
                    uint32_t orderId = eventData["orderId"] | 0;
                    float amount = eventData["amount"] | 0.0f;
                    
                    // Map document -> destinationDocument and accountNumber -> destinationAccount
                    // for POSController to consume seamlessly without breaking the HTTP path
                    eventData["destinationDocument"] = eventData["document"];
                    eventData["destinationAccount"] = eventData["accountNumber"];
                    // ticketId is preserved in eventData automatically
                    
                    String payloadStr;
                    serializeJson(eventData, payloadStr);
                    
                    String opIdStr = "OP-" + String(millis()) + "-" + String(orderId);
                    OperacionPago* op = paymentBuffer.addOperation(opIdStr.c_str(), orderId, amount, payloadStr.c_str(), "WSS");
                    
                    if (op != nullptr) {
                        QueueItem item;
                        strlcpy(item.operationId, op->operationId, sizeof(item.operationId));
                        if (xQueueSend(orderQueue, &item, (TickType_t)0) == pdPASS) {
                            String resp = "[\"pos_payment_accepted\", {\"operationId\":\"" + opIdStr + "\", \"status\":\"accepted\"}]";
                            globalComms->socketIO.sendEVENT(resp.c_str());
                        } else {
                            paymentBuffer.updateState(opIdStr.c_str(), ERROR, "Cola llena");
                            String err = "[\"pos_payment_error\", {\"operationId\":\"" + opIdStr + "\", \"status\":\"error\"}]";
                            globalComms->socketIO.sendEVENT(err.c_str());
                        }
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
        socketIO.loop();
        
        // Workaround: Esperar a que la libreria complete el handshake interno de Socket.IO antes de emitir eventos
        if (pendingWsAuth && (millis() - wsAuthTimer > 1500)) {
            pendingWsAuth = false;
            Serial.println("[SocketIO] Handshake completo. Enviando autenticacion pos:join...");
            JsonDocument joinDoc;
            joinDoc.add("pos:join");
            JsonObject auth = joinDoc.add<JsonObject>();
            auth["posApiKey"] = posApiKey;
            String joinMsg;
            serializeJson(joinDoc, joinMsg);
            socketIO.sendEVENT(joinMsg.c_str());
        }
    }
}

void CommsHandler::sendSocketIOEvent(const String& eventName, const String& jsonPayload) {
    if (wsEnabled) {
        String sMsg = "[\"" + eventName + "\"," + jsonPayload + "]";
        socketIO.sendEVENT(sMsg.c_str());
    }
}
