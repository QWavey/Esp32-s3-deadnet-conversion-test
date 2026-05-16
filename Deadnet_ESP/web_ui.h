#ifndef WEB_UI_H
#define WEB_UI_H

#include <Arduino.h>

const char INDEX_HTML[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Deadnet ESP32</title>
    <link rel="stylesheet" href="style.css">
    <link rel="icon" href="data:,">
</head>
<body>
    <div class="hero">
        <h1>Deadnet</h1>
        <p>Advanced ESP32 MITM Framework</p>
    </div>

    <div class="tabs">
        <button class="tab-btn active" data-tab="attack" onclick="showTab('attack')">Attack</button>
        <button class="tab-btn" data-tab="listener" onclick="showTab('listener')">ESP Listener</button>
    </div>

    <!-- ATTACK TAB -->
    <div id="tab-attack" class="tab-content active">
        <div class="grid">
            <!-- STATUS CARD -->
            <div class="card">
                <h2>System Status</h2>
                <div class="pill-container">
                    <span id="wifiPill" class="pill pill-red"><span class="dot pulse"></span>Disconnected</span>
                    <span id="atkPill" class="pill pill-orange"><span class="dot"></span>Idle</span>
                </div>
                <div class="stat-container">
                    <div class="stat-row"><span>Target SSID</span><span class="stat-val" id="stSSID">–</span></div>
                    <div class="stat-row"><span>My IP</span><span class="stat-val" id="stIP">–</span></div>
                    <div class="stat-row"><span>Gateway</span><span class="stat-val" id="stGW">–</span></div>
                    <div class="stat-row"><span>Packets</span><span class="stat-val" id="stPkts">0</span></div>
                    <div class="stat-row"><span>Hosts</span><span class="stat-val" id="stHosts">0</span></div>
                </div>
            </div>

            <!-- WIFI CONFIG CARD -->
            <div class="card">
                <h2>Network Setup</h2>
                <div class="input-group">
                    <label>Target SSID</label>
                    <div class="row">
                        <input type="text" id="inSSID" placeholder="SSID">
                        <button class="btn btn-ghost btn-sm" id="scanBtn" onclick="doScan()">Scan</button>
                    </div>
                </div>
                <div id="scanResults" class="net-list" style="display:none;"></div>
                <div class="input-group">
                    <label>Password</label>
                    <input type="password" id="inPass" placeholder="Password">
                </div>
                <div class="row">
                    <button class="btn btn-green" id="connectBtn" onclick="doConnect()">Connect</button>
                    <button class="btn btn-ghost btn-sm" onclick="doDisconnect()">&#10006;</button>
                </div>
                <div class="section-header">
                    <div class="sect-title">Saved</div>
                    <button class="btn btn-ghost btn-sm text-red" onclick="clearSaved()">Clear</button>
                </div>
                <div id="savedNets" class="net-list small"></div>
            </div>

            <!-- ATTACK CARD -->
            <div class="card">
                <h2>Attack Control</h2>
                <div class="input-group">
                    <label>Methods</label>
                    <div class="checkbox-group">
                        <label class="cb-label"><input type="checkbox" class="mode-cb" value="arp" checked> ARP Poison</label>
                        <label class="cb-label"><input type="checkbox" class="mode-cb" value="dns"> DNS Spoof</label>
                        <label class="cb-label"><input type="checkbox" class="mode-cb" value="deauth"> Deauth (Kill)</label>
                        <label class="cb-label"><input type="checkbox" class="mode-cb" value="ra"> IPv6 RA</label>
                    </div>
                </div>
                <div class="input-group">
                    <label class="cb-label"><input type="checkbox" id="blindMode"> Blind Sweep (No Discovery)</label>
                </div>
                
                <button id="atkBtnMain" class="btn btn-red full-width" onclick="toggleAttack('main')">&#9654; Start Attack</button>
                
                <div class="section-header">
                    <div class="sect-title">Hosts</div>
                    <div class="row">
                        <button class="btn btn-ghost btn-sm" onclick="selectAllHosts(true)">All</button>
                        <button class="btn btn-ghost btn-sm" onclick="selectAllHosts(false)">None</button>
                    </div>
                </div>
                <div id="hostList" class="host-list">
                    <div class="empty-state">No hosts found. Scan or start attack.</div>
                </div>
                <button class="btn btn-ghost btn-sm full-width" onclick="refreshHosts()">&#8635; Refresh List</button>
            </div>

            <!-- LOGS CARD -->
            <div class="card full-width">
                <h2>System Logs</h2>
                <div id="logs" class="logs">[info] Deadnet ready.</div>
            </div>
        </div>
    </div>

    <!-- LISTENER TAB -->
    <div id="tab-listener" class="tab-content">
        <div class="grid">
            <div class="card full-width">
                <div class="section-header">
                    <h2>Traffic Interception</h2>
                    <div class="row">
                        <button class="btn btn-ghost btn-sm" onclick="doScanHosts()">Discover Devices</button>
                        <button class="btn btn-ghost btn-sm" onclick="refreshHosts()">Refresh UI</button>
                    </div>
                </div>
                <div class="input-group">
                    <label>Capture Mode</label>
                    <div class="checkbox-group">
                        <label class="cb-label"><input type="checkbox" id="modeSniff" checked> Passive Sniffing</label>
                        <label class="cb-label"><input type="checkbox" id="modeMitm"> Force MITM</label>
                    </div>
                </div>
                <div class="row">
                    <button id="atkBtnSniff" class="btn btn-red" onclick="toggleAttack('sniff')">&#9654; Start Capture</button>
                    <button class="btn btn-ghost" onclick="startAll()">Full MITM (Extreme)</button>
                </div>
            </div>

            <div class="card full-width">
                <div class="tabs sub-tabs">
                    <button class="tab-btn active" id="st-session" onclick="showSubTab('session')">Session Logs</button>
                    <button class="tab-btn" id="st-devices" onclick="showSubTab('devices')">Target Devices</button>
                </div>

                <div id="sub-session" class="sub-tab-content active">
                    <div class="section-header">
                        <h2>Captured Packets</h2>
                        <button class="btn btn-ghost btn-sm text-red" onclick="clearSniff()">Clear</button>
                    </div>
                    <div id="sniffList" class="sniff-list">
                        <div class="empty-state">Listening for traffic...</div>
                    </div>
                </div>

                <div id="sub-devices" class="sub-tab-content">
                    <div class="section-header">
                        <h2>Target Selection</h2>
                        <button class="btn btn-green btn-sm" onclick="runTargeted()">Run Selected</button>
                    </div>
                    <div id="deviceList" class="sniff-list">
                        <div class="empty-state">Perform a scan to see devices.</div>
                    </div>
                </div>
            </div>
        </div>
    </div>

    <script src="main.js"></script>
</body>
</html>
)rawhtml";

const char STYLE_CSS[] PROGMEM = R"rawcss(
/* Material Design 3 (Google Pixel) - Deep Dark Theme */
@import url('https://fonts.googleapis.com/css2?family=Google+Sans:wght@400;500;700&family=Roboto:wght@400;500&display=swap');

:root {
  --md-sys-color-primary: #d3e3fd;
  --md-sys-color-background: #111111;
  --md-sys-color-surface: #1e1e1e;
  --md-sys-color-surface-variant: #333333;
  --md-sys-color-on-surface: #e3e3e3;
  --md-sys-color-on-surface-variant: #b0b0b0;
  --md-sys-color-error: #f2b8b5;
  --md-sys-color-primary-fixed: #a8c7fa;
  
  --r: var(--md-sys-color-error);
  --g: #a2f9b1;
  --o: #ffb4a1;
  --bg: var(--md-sys-color-background);
  --card: var(--md-sys-color-surface);
  --text: var(--md-sys-color-on-surface);
  --sub: var(--md-sys-color-on-surface-variant);
}

* { box-sizing: border-box; margin: 0; padding: 0; font-family: 'Google Sans', 'Roboto', sans-serif; -webkit-tap-highlight-color: transparent; }

body {
  background: var(--bg);
  color: var(--text);
  min-height: 100vh;
  padding: 1rem;
  line-height: 1.5;
  background-image: radial-gradient(circle at 50% 50%, rgba(255,255,255,0.03) 0%, transparent 100%);
  background-attachment: fixed;
}

/* Animations */
@keyframes fadeIn { from { opacity: 0; transform: translateY(8px); } to { opacity: 1; transform: translateY(0); } }
@keyframes pulse { 0%, 100% { opacity: 1; transform: scale(1); } 50% { opacity: 0.5; transform: scale(0.9); } }
@keyframes ripple-effect { to { transform: scale(4); opacity: 0; } }

.ripple { position: absolute; background: rgba(255,255,255,0.25); border-radius: 50%; transform: scale(0); animation: ripple-effect 0.5s linear; pointer-events: none; z-index: 10; }

h2 { font-size: 1.1rem; font-weight: 500; color: var(--md-sys-color-primary-fixed); margin-bottom: 1rem; }
.hero { text-align: center; padding: 2.5rem 1rem 1.5rem; animation: fadeIn 0.5s ease-out; }
.hero h1 { font-size: 2.8rem; font-weight: 700; color: var(--md-sys-color-primary-fixed); letter-spacing: -1px; }
.hero p { color: var(--sub); font-size: 1rem; margin-top: 0.3rem; }

.grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(320px, 1fr)); gap: 1rem; max-width: 1200px; margin: 0 auto; }
.card { background: var(--card); border-radius: 24px; padding: 1.25rem; transition: background 0.2s; animation: fadeIn 0.6s ease-out; overflow: hidden; position: relative; }
.card:hover { background: #222222; }
.card.full-width { grid-column: 1 / -1; }

.tabs { display: flex; justify-content: center; margin: 0 auto 2rem; background: var(--md-sys-color-surface-variant); border-radius: 9999px; width: fit-content; padding: 4px; }
.tab-btn { padding: 0.7rem 2rem; border-radius: 9999px; background: transparent; color: var(--sub); border: none; cursor: pointer; font-weight: 500; font-size: 0.9rem; transition: all 0.2s; }
.tab-btn.active { background: var(--md-sys-color-primary-fixed); color: #000; }
.tab-content { display: none; }
.tab-content.active { display: block; animation: fadeIn 0.3s; }

.sub-tabs { background: rgba(255,255,255,0.05); margin-bottom: 1rem; padding: 2px; }
.sub-tabs .tab-btn { padding: 0.5rem 1.2rem; font-size: 0.8rem; }
.sub-tab-content { display: none; }
.sub-tab-content.active { display: block; }

.pill-container { display: flex; gap: 0.6rem; margin-bottom: 1rem; }
.pill { display: inline-flex; align-items: center; gap: 0.4rem; padding: 0.4rem 1rem; border-radius: 9999px; font-size: 0.8rem; font-weight: 500; }
.pill-red { background: rgba(242,184,181,0.1); color: var(--r); }
.pill-green { background: rgba(162,249,177,0.1); color: var(--g); }
.pill-orange { background: rgba(255,180,161,0.1); color: var(--o); }
.dot { width: 8px; height: 8px; border-radius: 50%; background: currentColor; }
.dot.pulse { animation: pulse 1.5s infinite; }

label { display: block; font-size: 0.85rem; font-weight: 500; color: var(--md-sys-color-primary-fixed); margin-bottom: 0.4rem; margin-left: 0.3rem; }
.input-group { margin-bottom: 1rem; }
input, select { width: 100%; padding: 0.9rem 1rem; background: var(--md-sys-color-surface-variant); border: 2px solid transparent; border-radius: 14px; color: var(--text); outline: none; font-size: 0.95rem; }
input:focus { border-color: var(--md-sys-color-primary-fixed); background: #3d3d3d; }

.checkbox-group { display: flex; flex-wrap: wrap; gap: 0.6rem; background: rgba(0,0,0,0.2); padding: 0.8rem; border-radius: 14px; }
.cb-label { display: flex; align-items: center; gap: 0.6rem; font-size: 0.9rem; color: var(--text); cursor: pointer; margin: 0; padding: 0.3rem 0.6rem; border-radius: 8px; transition: background 0.2s; }
.cb-label:hover { background: rgba(255,255,255,0.05); }
.cb-label input { width: 18px; height: 18px; cursor: pointer; }

.row { display: flex; gap: 0.6rem; align-items: center; }
.row > * { flex: 1; }
.row > .btn-sm { flex: 0 0 auto; }

.btn { position: relative; overflow: hidden; padding: 0.9rem 1.2rem; border-radius: 9999px; font-weight: 500; cursor: pointer; border: none; transition: all 0.2s; display: inline-flex; align-items: center; justify-content: center; gap: 0.5rem; }
.btn:active { transform: scale(0.97); }
.btn-green { background: var(--md-sys-color-primary-fixed); color: #000; }
.btn-red { background: var(--md-sys-color-error); color: #000; }
.btn-ghost { background: transparent; color: var(--md-sys-color-primary-fixed); border: 1px solid var(--md-sys-color-primary-fixed); }
.btn-sm { padding: 0.5rem 1rem; font-size: 0.85rem; width: auto !important; }
.full-width { width: 100% !important; }

.net-list, .host-list, .sniff-list { max-height: 200px; overflow-y: auto; border-radius: 14px; background: rgba(0,0,0,0.15); margin-top: 0.5rem; }
.net-list.small { max-height: 120px; }
.sniff-list { height: 350px; background: #000; padding: 0.5rem; }

.item { display: flex; justify-content: space-between; align-items: center; padding: 0.9rem; border-bottom: 1px solid rgba(255,255,255,0.03); cursor: pointer; transition: background 0.2s; position: relative; overflow: hidden; }
.item:hover { background: rgba(255,255,255,0.04); }
.item-check { width: 20px; height: 20px; margin-right: 10px; cursor: pointer; }

.meta { font-size: 0.75rem; color: var(--sub); font-family: 'Roboto Mono', monospace; }
.stat-row { display: flex; justify-content: space-between; padding: 0.5rem 0; border-bottom: 1px solid rgba(255,255,255,0.02); font-size: 0.9rem; }
.stat-val { font-weight: 500; color: #fff; }

.logs { background: #000; padding: 1rem; border-radius: 18px; font-family: 'Roboto Mono', monospace; font-size: 0.8rem; height: 180px; overflow-y: auto; color: var(--g); line-height: 1.5; }
.sniff-item { padding: 0.8rem; border-bottom: 1px solid rgba(255,255,255,0.05); display: flex; flex-direction: column; gap: 0.3rem; animation: fadeIn 0.3s; }
.tag { padding: 2px 6px; border-radius: 4px; font-size: 0.65rem; font-weight: 700; text-transform: uppercase; }
.tag-http { background: var(--md-sys-color-primary-fixed); color: #000; }
.tag-dns { background: var(--o); color: #000; }
.play-btn { width: 32px; height: 32px; border-radius: 50%; background: rgba(162,249,177,0.1); color: var(--g); border: none; cursor: pointer; display: flex; align-items: center; justify-content: center; }

/* Mobile Optimizations */
@media (max-width: 600px) {
  .hero h1 { font-size: 2.2rem; }
  .grid { grid-template-columns: 1fr; }
  .tab-btn { padding: 0.6rem 1.2rem; font-size: 0.85rem; }
}

::-webkit-scrollbar { width: 6px; }
::-webkit-scrollbar-thumb { background: #333; border-radius: 10px; }
)rawcss";

const char MAIN_JS[] PROGMEM = R"rawjs(
let attacking = false; let connected = false; let hostCache = {};
const esc = (s) => String(s || '').replace(/[&<>"']/g, m => ({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'})[m]);

function pollStatus() {
    fetch('/api/status').then(r => r.json()).then(d => {
        connected = d.connected; attacking = d.running;
        const wp = document.getElementById('wifiPill');
        wp.className = 'pill ' + (d.connected ? 'pill-green' : 'pill-red');
        wp.innerHTML = `<span class="dot${d.connected?'':' pulse'}"></span>${d.connected?esc(d.ssid):'Disconnected'}`;
        const ap = document.getElementById('atkPill');
        ap.className = 'pill ' + (d.running ? 'pill-red' : 'pill-orange');
        ap.innerHTML = `<span class="dot${d.running?' pulse':''}"></span>${d.running?'ATTACKING':'Idle'}`;
        ['stSSID','stIP','stGW','stPkts','stHosts'].forEach(id => {
            let val = d[id.replace('st','').toLowerCase()];
            document.getElementById(id).textContent = val || (id==='stPkts'||id==='stHosts'?'0':'–');
        });
        
        [document.getElementById('atkBtnMain'), document.getElementById('atkBtnSniff')].forEach(b => {
            if(!b) return;
            b.innerHTML = d.running ? 'Stop Attack' : '&#9654; Start Attack';
            b.className = d.running ? 'btn btn-ghost full-width' : 'btn btn-red full-width';
            b.disabled = !d.connected && !d.running;
        });
    }).catch(()=>{});
}

function pollSniff() {
    if (!attacking) return;
    fetch('/api/sniff').then(r => r.json()).then(d => {
        const el = document.getElementById('sniffList');
        if (!d.logs || d.logs.length === 0) { 
            if(el.querySelector('.empty-state')) return;
            el.innerHTML = '<div class="empty-state">No data captured yet.</div>'; 
            return; 
        }
        el.innerHTML = d.logs.map(l => {
            const name = hostCache[l.src] || l.src;
            const tagCls = l.type.toLowerCase().includes('dns') ? 'tag-dns' : 'tag-http';
            return `<div class="sniff-item">
                <div style="display:flex;justify:space-between;align-items:center;">
                    <span class="tag ${tagCls}">${esc(l.type)}</span>
                    <span style="font-size:0.75rem;opacity:0.6;margin-left:auto;">${new Date(l.time).toLocaleTimeString()}</span>
                </div>
                <div style="font-weight:500;font-size:0.85rem;margin-top:2px;">${esc(name)}</div>
                <div class="meta" style="color:var(--md-sys-color-primary-fixed);background:rgba(255,255,255,0.03);padding:4px 8px;border-radius:6px;margin-top:4px;">${esc(l.content)}</div>
            </div>`;
        }).reverse().join('');
    }).catch(()=>{});
}

function pollLogs() {
    if (!connected && !attacking) return;
    fetch('/api/logs').then(r => r.json()).then(d => {
        const el = document.getElementById('logs');
        el.innerHTML = d.logs.map(l => {
            let cls = l.includes('[error]') ? 'log-err' : (l.includes('[warning]') ? 'log-warn' : '');
            return cls ? `<span class="${cls}">${esc(l)}</span>` : esc(l);
        }).join('<br>');
        el.scrollTop = el.scrollHeight;
    }).catch(()=>{});
}

function showTab(t) {
    document.querySelectorAll('.tab-content').forEach(c => c.classList.remove('active'));
    document.querySelectorAll('.tab-btn:not(.sub-tabs .tab-btn)').forEach(b => b.classList.remove('active'));
    document.getElementById('tab-' + t).classList.add('active');
    document.querySelector(`.tab-btn[data-tab="${t}"]`).classList.add('active');
    if(t === 'listener') refreshHosts();
}
function showSubTab(t) {
    document.querySelectorAll('.sub-tab-content').forEach(c => c.classList.remove('active'));
    document.querySelectorAll('.sub-tabs .tab-btn').forEach(b => b.classList.remove('active'));
    document.getElementById('sub-' + t).classList.add('active');
    document.getElementById('st-' + t).classList.add('active');
}

function selectAllHosts(v) {
    document.querySelectorAll('.host-check').forEach(c => c.checked = v);
}

function doScan() {
    const btn = document.getElementById('scanBtn'); btn.disabled = true; btn.textContent = '...';
    document.getElementById('scanResults').style.display = 'block';
    document.getElementById('scanResults').innerHTML = '<div class="empty-state">Scanning...</div>';
    fetch('/api/scan').then(r => r.json()).then(d => { if(d.success) setTimeout(pollScan, 2000); });
}
function pollScan() {
    fetch('/api/scan-results').then(r => r.json()).then(d => {
        if(d.status === 'scanning') { setTimeout(pollScan, 1000); return; }
        document.getElementById('scanBtn').disabled = false; document.getElementById('scanBtn').textContent = 'Scan';
        if(d.status === 'complete') {
            const el = document.getElementById('scanResults');
            el.innerHTML = d.networks.sort((a,b)=>b.rssi-a.rssi).map(n => `<div class="item" onclick="selectNet('${esc(n.ssid)}')"><div><div style="font-weight:500;">${esc(n.ssid)}</div><div class="meta">ch${n.channel} &bull; ${n.rssi}dBm</div></div></div>`).join('');
        }
    });
}
function selectNet(s) { document.getElementById('inSSID').value = s; document.getElementById('scanResults').style.display = 'none'; }

function doConnect() {
    const s = document.getElementById('inSSID').value; const p = document.getElementById('inPass').value;
    fetch(`/api/connect?ssid=${encodeURIComponent(s)}&pass=${encodeURIComponent(p)}`).then(() => pollStatus());
}
function doDisconnect() { fetch('/api/disconnect').then(() => pollStatus()); }

function doScanHosts() {
    fetch('/api/start?mode=0').then(() => { // Starting "Discovery" mode
        setTimeout(refreshHosts, 2000);
    });
}

function toggleAttack(src) {
    if (attacking) fetch('/api/stop').then(() => pollStatus());
    else {
        let modes = [];
        if(src === 'sniff') {
            if(document.getElementById('modeSniff').checked) modes.push('sniff');
            if(document.getElementById('modeMitm').checked) { modes.push('arp'); modes.push('dns'); }
        } else {
            document.querySelectorAll('.mode-cb:checked').forEach(c => modes.push(c.value));
            if(document.getElementById('blindMode').checked) modes.push('blind');
        }
        let modeVal = 0;
        const map = {'arp':1,'ra':2,'deauth':4,'blind':8,'dns':16,'sniff':32};
        modes.forEach(m => modeVal |= map[m]);
        
        let targets = Array.from(document.querySelectorAll('.host-check:checked')).map(c => c.value);
        let url = '/api/start?mode=' + modeVal;
        if (targets.length > 0) url += '&targets=' + encodeURIComponent(targets.join(','));
        fetch(url).then(() => pollStatus());
    }
}

function runTargeted() {
    const sel = Array.from(document.querySelectorAll('.dev-check:checked')).map(c => c.value);
    if(sel.length===0) return alert('Select devices');
    let url = '/api/start?mode=49'; // ARP(1) + DNS(16) + SNIFF(32)
    url += '&targets=' + encodeURIComponent(sel.join(','));
    fetch(url).then(() => { showTab('attack'); pollStatus(); });
}
function playTarget(ip) {
    fetch('/api/start?mode=49&targets=' + encodeURIComponent(ip)).then(() => { showTab('attack'); pollStatus(); });
}

function refreshHosts() {
    fetch('/api/hosts').then(r => r.json()).then(d => {
        d.hosts.forEach(h => { if(h.hostname) hostCache[h.ip] = h.hostname; });
        const el = document.getElementById('hostList');
        el.innerHTML = d.hosts.map(h => `<div class="item" onclick="this.querySelector('input').click()"><input type="checkbox" class="host-check" value="${esc(h.ip)}" onclick="event.stopPropagation()"><div><div style="font-weight:500;">${esc(h.hostname || h.ip)}</div><div class="meta">${esc(h.ip)} &bull; ${esc(h.mac)}</div></div></div>`).join('');
        const devEl = document.getElementById('deviceList');
        if(devEl) devEl.innerHTML = d.hosts.map(h => `<div class="item"><input type="checkbox" class="dev-check" value="${esc(h.ip)}"><div><div style="font-weight:500;">${esc(h.hostname || h.ip)}</div><div class="meta">${esc(h.ip)}</div></div><button class="play-btn" onclick="playTarget('${esc(h.ip)}')"><svg viewBox="0 0 24 24" width="18" height="18" fill="currentColor"><path d="M8 5v14l11-7z"/></svg></button></div>`).join('');
    });
}

document.addEventListener('mousedown', e => {
    const t = e.target.closest('.btn, .pill, .item, .tab-btn'); if (!t) return;
    if(window.getComputedStyle(t).position === 'static') t.style.position = 'relative';
    t.style.overflow = 'hidden';
    const r = t.getBoundingClientRect(); const s = document.createElement('span'); s.className = 'ripple';
    const sz = Math.max(r.width, r.height) * 2; s.style.width = s.style.height = sz + 'px';
    s.style.left = (e.clientX - r.left - sz/2) + 'px'; s.style.top = (e.clientY - r.top - sz/2) + 'px';
    t.appendChild(s); setTimeout(() => s.remove(), 600);
});

setInterval(pollStatus, 2000); setInterval(pollLogs, 3000); setInterval(pollSniff, 4000);
pollStatus(); refreshHosts();
)rawjs";

#endif
