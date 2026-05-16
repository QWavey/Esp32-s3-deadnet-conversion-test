// Deadnet ESP32 - Application Logic
let attacking = false;
let connected = false;

// HTML Escaping
const esc = (str) => {
    if (!str) return '';
    return String(str).replace(/[&<>"']/g, m => ({
        '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;'
    })[m]);
};

// ── Polling ──────────────────────────────────────────────────────────────────
function pollStatus() {
    fetch('/api/status')
        .then(r => r.json())
        .then(d => {
            connected = d.connected;
            attacking = d.running;

            const wp = document.getElementById('wifiPill');
            wp.className = 'pill ' + (d.connected ? 'pill-green' : 'pill-red');
            wp.innerHTML = `<span class="dot${d.connected ? '' : ' pulse'}"></span>${d.connected ? esc(d.ssid) : 'Disconnected'}`;

            const ap = document.getElementById('atkPill');
            ap.className = 'pill ' + (d.running ? 'pill-red' : 'pill-orange');
            ap.innerHTML = `<span class="dot${d.running ? ' pulse' : ''}"></span>${d.running ? 'ATTACKING' : 'Idle'}`;

            document.getElementById('stSSID').textContent = d.ssid || '–';
            document.getElementById('stIP').textContent = d.ip || '–';
            document.getElementById('stGW').textContent = d.gateway || '–';
            document.getElementById('stGWMAC').textContent = d.gatewayMac || '–';
            document.getElementById('stPkts').textContent = d.packets;
            document.getElementById('stCycles').textContent = d.cycles;
            document.getElementById('stHosts').textContent = d.hosts;

            // Update all start buttons
            const btnMain = document.getElementById('atkBtnMain');
            const btnSniff = document.getElementById('atkBtnSniff');
            
            const updateBtn = (b) => {
                if (!b) return;
                if (d.running) {
                    b.innerHTML = 'Stop Attack';
                    b.className = 'btn btn-ghost full-width';
                } else {
                    b.innerHTML = '&#9654; Start Attack';
                    b.className = 'btn btn-red full-width';
                }
                b.disabled = !d.connected && !d.running;
            };

            updateBtn(btnMain);
            updateBtn(btnSniff);
        })
        .catch(() => { });
}

function pollSniff() {
    fetch('/api/sniff')
        .then(r => r.json())
        .then(d => {
            const el = document.getElementById('sniffList');
            if (!d.logs || d.logs.length === 0) {
                if (!el.querySelector('.empty-state')) {
                    el.innerHTML = '<div class="empty-state">No data captured yet.</div>';
                }
                return;
            }
            const html = d.logs.map(l => {
                const name = hostCache[l.src] || l.src;
                const tag = l.type.toLowerCase();
                return `
                <div class="sniff-item">
                    <div class="sniff-meta">
                        <span class="sniff-tag tag-${tag}">${esc(l.type)}</span>
                        <span class="sniff-src">${esc(name)}</span>
                        <span style="margin-left:auto;opacity:0.5;">${new Date(l.time).toLocaleTimeString()}</span>
                    </div>
                    <div class="sniff-content">${esc(l.content)}</div>
                </div>`;
            }).reverse().join('');
            if (el.innerHTML !== html) {
                el.innerHTML = html;
                el.scrollTop = 0;
            }
        })
        .catch(() => { });
}

let hostCache = {};

function pollLogs() {
    fetch('/api/logs')
        .then(r => r.json())
        .then(d => {
            const el = document.getElementById('logs');
            const html = d.logs.map(l => {
                let cls = '';
                if (l.includes('[error]')) cls = 'log-err';
                if (l.includes('[warning]')) cls = 'log-warn';
                return cls ? `<span class="${cls}">${esc(l)}</span>` : esc(l);
            }).join('<br>');
            if (el.innerHTML !== html) {
                el.innerHTML = html;
                el.scrollTop = el.scrollHeight;
            }
        })
        .catch(() => { });
}

setInterval(pollStatus, 1500);
setInterval(pollLogs, 2000);
setInterval(pollSniff, 3000);
pollStatus();
pollLogs();
pollSniff();

// ── Tabs ───────────────────────────────────────────────────────────────────
function showTab(tab) {
    document.querySelectorAll('.tab-content').forEach(c => c.classList.remove('active'));
    document.querySelectorAll('.tab-btn:not(.sub-tabs .tab-btn)').forEach(b => b.classList.remove('active'));
    document.getElementById('tab-' + tab).classList.add('active');
    document.querySelector(`.tab-btn[data-tab="${tab}"]`).classList.add('active');
}

function showSubTab(tab) {
    document.querySelectorAll('.sub-tab-content').forEach(c => c.classList.remove('active'));
    document.querySelectorAll('.sub-tabs .tab-btn').forEach(b => b.classList.remove('active'));
    document.getElementById('sub-' + tab).classList.add('active');
    document.querySelector(`.sub-tabs .tab-btn[onclick="showSubTab('${tab}')"]`).classList.add('active');
}

// ── WiFi Scan ────────────────────────────────────────────────────────────────
function doScan() {
    const btn = document.getElementById('scanBtn');
    btn.disabled = true;
    btn.textContent = '...';
    const res = document.getElementById('scanResults');
    res.style.display = 'block';
    res.innerHTML = '<div class="empty-state">Scanning...</div>';

    fetch('/api/scan')
        .then(r => r.json())
        .then(d => {
            if (d.success) {
                setTimeout(pollScan, 2500);
            } else {
                res.innerHTML = `<div class="empty-state" style="color:var(--r);">${esc(d.error || 'Scan failed')}</div>`;
                btn.disabled = false; btn.textContent = 'Scan';
            }
        })
        .catch(() => { btn.disabled = false; btn.textContent = 'Scan'; });
}

function pollScan() {
    fetch('/api/scan-results')
        .then(r => r.json())
        .then(d => {
            if (d.status === 'scanning') { setTimeout(pollScan, 1000); return; }
            const btn = document.getElementById('scanBtn');
            btn.disabled = false; btn.textContent = 'Scan';
            if (d.status === 'complete') renderScanResults(d.networks);
            else document.getElementById('scanResults').innerHTML = '<div class="empty-state" style="color:var(--r);">Scan error</div>';
        })
        .catch(() => { document.getElementById('scanBtn').disabled = false; });
}

function renderScanResults(nets) {
    const el = document.getElementById('scanResults');
    if (!nets || nets.length === 0) {
        el.innerHTML = '<div class="empty-state">No networks found.</div>'; return;
    }
    const enc = { 0: 'Open', 1: 'WEP', 2: 'WPA', 3: 'WPA2', 4: 'WPA/WPA2', 5: 'WPA2-Ent', 6: 'WPA3', 7: 'WPA2/3' };
    const sorted = nets.sort((a, b) => b.rssi - a.rssi);
    
    el.innerHTML = sorted.map(n => {
        const pct = Math.min(100, Math.max(0, 2 * (n.rssi + 100)));
        const sec = enc[n.encryption] || '?';
        return `<div class="net-item" onclick="selectNet('${esc(n.ssid)}')">
      <div>
        <div class="net-ssid">${esc(n.ssid)}</div>
        <div class="net-meta">${sec} &bull; ch${n.channel} &bull; ${n.rssi} dBm &bull; ${n.bssid}</div>
      </div>
      <div class="signal-bar"><div class="signal-fill" style="width:${pct}%"></div></div>
    </div>`;
    }).join('');
}

function selectNet(ssid) {
    document.getElementById('inSSID').value = ssid;
    document.getElementById('scanResults').style.display = 'none';
    document.getElementById('inPass').focus();
}

// ── WiFi Connect ─────────────────────────────────────────────────────────────
function doConnect() {
    const ssid = document.getElementById('inSSID').value.trim();
    const pass = document.getElementById('inPass').value;
    if (!ssid) { alert('Enter an SSID first.'); return; }
    const btn = document.getElementById('connectBtn');
    btn.disabled = true; btn.textContent = 'Connecting...';
    fetch(`/api/connect?ssid=${encodeURIComponent(ssid)}&pass=${encodeURIComponent(pass)}`)
        .then(r => r.json())
        .then(d => {
            btn.disabled = false;
            btn.textContent = 'Connect';
            if (d.success) {
                loadSaved();
                pollStatus();
            } else {
                alert('Connection failed: ' + (d.error || 'timeout'));
            }
        })
        .catch(() => { btn.disabled = false; btn.textContent = 'Connect'; });
}

function doDisconnect() {
    fetch('/api/disconnect').then(() => pollStatus());
}

function clearSaved() {
    if (!confirm('Clear all saved networks?')) return;
    fetch('/api/clear-networks').then(() => loadSaved());
}

// ── Saved Networks ────────────────────────────────────────────────────────────
function loadSaved() {
    fetch('/api/networks')
        .then(r => r.json())
        .then(d => {
            const el = document.getElementById('savedNets');
            if (!d.networks || d.networks.length === 0) {
                el.innerHTML = '<div class="empty-state">No saved networks.</div>'; return;
            }
            el.innerHTML = d.networks.map(n =>
                `<div class="net-item" onclick="quickConnect('${esc(n.ssid)}')">
          <span class="net-ssid">${esc(n.ssid)}</span>
          <span style="font-size:.85rem;color:var(--md-sys-color-primary-fixed);font-weight:500;">Connect</span>
        </div>`
            ).join('');
        });
}

function quickConnect(ssid) {
    document.getElementById('inSSID').value = ssid;
    doConnect();
}

// ── Attack ────────────────────────────────────────────────────────────────────
function toggleAttack(source, targets = null) {
    if (attacking) {
        fetch('/api/stop').then(r => r.json()).then(() => { pollStatus(); });
    } else {
        if (!connected) { alert('Connect to a WiFi network first!'); return; }
        let mode = 'arp';
        if (source === 'sniff') {
            mode = document.getElementById('attackModeSniff').value;
        } else {
            mode = document.getElementById('attackModeMain').value;
        }
        
        let url = '/api/start?mode=' + mode;
        if (targets && targets.length > 0) {
            url += '&targets=' + encodeURIComponent(targets.join(','));
        }

        fetch(url)
            .then(r => r.json())
            .then(d => {
                if (!d.success) alert('Start failed: ' + (d.error || 'unknown'));
                else { pollStatus(); pollSniff(); setTimeout(refreshHosts, 5000); }
            });
    }
}

function playTarget(ip) {
    const mode = document.getElementById('attackModeSniff').value;
    toggleAttack('sniff', [ip]);
}

function runTargeted() {
    const selected = Array.from(document.querySelectorAll('.device-check:checked')).map(c => c.value);
    if (selected.length === 0) { alert('Select at least one device!'); return; }
    toggleAttack('sniff', selected);
}

function startAll() {
    fetch('/api/start?mode=both')
        .then(r => r.json())
        .then(d => {
            if (!d.success) alert('Start failed: ' + (d.error || 'unknown'));
            else { pollStatus(); pollSniff(); setTimeout(refreshHosts, 5000); }
        });
}

function clearSniff() {
    if (!confirm('Clear sniffed data?')) return;
    fetch('/api/clear-sniff').then(() => pollSniff());
}

// ── Hosts ─────────────────────────────────────────────────────────────────────
function clearHostsUI() {
    if (!confirm('Clear discovered hosts?')) return;
    fetch('/api/clear-hosts').then(() => refreshHosts());
}

function refreshHosts() {
    fetch('/api/hosts')
        .then(r => r.json())
        .then(d => {
            const el = document.getElementById('hostList');
            if (!d.hosts || d.hosts.length === 0) {
                el.innerHTML = '<div class="empty-state">No hosts yet.</div>';
                return;
            }
            // Update cache
            d.hosts.forEach(h => {
                if (h.hostname) hostCache[h.ip] = h.hostname;
            });

            el.innerHTML = d.hosts.map(h =>
                `<div class="host-item">
          <div>
            <div style="font-weight:500;">${esc(h.ip)} ${h.hostname ? `<span style="color:var(--sub);font-size:.8rem;">(${esc(h.hostname)})</span>` : ''}</div>
            <div style="font-size:.8rem;color:var(--sub);font-family:monospace;">${esc(h.mac)}</div>
          </div>
          <div style="color:var(--g);font-weight:500;">${h.pingMs > 0 ? h.pingMs + 'ms' : ''}</div>
        </div>`
            ).join('');

            // Also update Device List in Sniff tab
            const devEl = document.getElementById('deviceList');
            if (devEl) {
                devEl.innerHTML = d.hosts.map(h => `
                <div class="device-item">
                    <input type="checkbox" class="device-check" value="${esc(h.ip)}">
                    <div>
                        <div style="font-weight:500;">${esc(h.hostname || h.ip)}</div>
                        <div style="font-size:0.8rem;color:var(--sub);font-family:monospace;">${esc(h.ip)} &bull; ${esc(h.mac)}</div>
                    </div>
                    <button class="play-btn" onclick="playTarget('${esc(h.ip)}')" title="Target this device">
                        <svg viewBox="0 0 24 24" width="20" height="20" fill="currentColor"><path d="M8 5v14l11-7z"/></svg>
                    </button>
                </div>
                `).join('');
            }
        });
}

// ── Ripple Effect ─────────────────────────────────────────────────────────────
document.addEventListener('mousedown', function (e) {
    const target = e.target.closest('.btn, .pill, .net-item, .host-item, .tab-btn');
    if (!target) return;

    // Ensure target is relative for absolute ripple positioning
    if (window.getComputedStyle(target).position === 'static') {
        target.style.position = 'relative';
    }
    target.style.overflow = 'hidden';

    const rect = target.getBoundingClientRect();
    const ripple = document.createElement('span');
    ripple.className = 'ripple';
    
    const size = Math.max(rect.width, rect.height) * 2;
    ripple.style.width = ripple.style.height = size + 'px';
    
    // Position relative to the element center
    const x = e.clientX - rect.left - size / 2;
    const y = e.clientY - rect.top - size / 2;
    
    ripple.style.left = x + 'px';
    ripple.style.top = y + 'px';
    
    target.appendChild(ripple);
    
    setTimeout(() => {
        ripple.remove();
    }, 600);
});

// Init
loadSaved();
refreshHosts();
pollStatus();
