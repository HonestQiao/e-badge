#ifndef INDEX_HTML_H
#define INDEX_HTML_H

const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>电子工牌管理系统</title>
<style>
* { margin:0; padding:0; box-sizing:border-box; }
body { font-family:-apple-system,BlinkMacSystemFont,sans-serif; background:#f5f5f5; padding:20px; }
.container { max-width:600px; margin:0 auto; background:#fff; border-radius:12px; box-shadow:0 2px 12px rgba(0,0,0,0.1); overflow:hidden; }
.header { background:#2196F3; color:#fff; padding:20px; text-align:center; }
.header h1 { font-size:20px; font-weight:500; }
.tabs { display:flex; border-bottom:1px solid #e0e0e0; }
.tab { flex:1; padding:15px; text-align:center; cursor:pointer; background:#f9f9f9; border:none; font-size:15px; }
.tab.active { background:#fff; border-bottom:3px solid #2196F3; color:#2196F3; font-weight:500; }
.content { padding:20px; }
.section { display:none; }
.section.active { display:block; }
.record-item { padding:15px; border-bottom:1px solid #eee; display:flex; justify-content:space-between; align-items:center; }
.record-item:last-child { border-bottom:none; }
.record-info h3 { font-size:16px; color:#333; margin-bottom:4px; }
.record-info p { font-size:13px; color:#999; }
.record-status { text-align:right; }
.record-status .time { font-size:14px; color:#2196F3; font-weight:500; }
.record-status .rssi { font-size:12px; color:#999; margin-top:2px; }
.empty { text-align:center; padding:40px; color:#999; }
.form-group { margin-bottom:18px; }
.form-group label { display:block; margin-bottom:6px; font-size:14px; color:#555; font-weight:500; }
.form-group input, .form-group select {
  width:100%; padding:12px 15px; border:1px solid #ddd; border-radius:8px;
  font-size:15px; -webkit-appearance:none; background:#fff;
}
.form-group input:focus, .form-group select:focus { outline:none; border-color:#2196F3; }
.btn {
  width:100%; padding:14px; background:#2196F3; color:#fff; border:none; border-radius:8px;
  font-size:16px; cursor:pointer; transition:background 0.2s;
}
.btn:hover { background:#1976D2; }
.btn:disabled { background:#ccc; cursor:not-allowed; }
.status-msg { margin-top:15px; padding:12px; border-radius:8px; text-align:center; font-size:14px; display:none; }
.status-msg.success { background:#e8f5e9; color:#2e7d32; display:block; }
.status-msg.error { background:#ffebee; color:#c62828; display:block; }
.device-list { margin-bottom:15px; }
.device-tag {
  display:inline-block; padding:8px 16px; margin:5px; background:#e3f2fd; color:#1976D2;
  border-radius:20px; font-size:14px; cursor:pointer; border:2px solid transparent;
}
.device-tag:hover { border-color:#2196F3; }
.device-tag.selected { background:#2196F3; color:#fff; }
.badge-preview {
  background:#f5f5f5; border-radius:8px; padding:15px; margin-top:15px;
  border-left:4px solid #2196F3;
}
.badge-preview h4 { font-size:14px; color:#666; margin-bottom:8px; }
.badge-preview p { font-size:15px; color:#333; margin:4px 0; }
</style>
</head>
<body>
<div class="container">
  <div class="header">
    <h1>电子工牌管理系统</h1>
  </div>
  <div class="tabs">
    <button class="tab active" onclick="showTab('records')">打卡记录</button>
    <button class="tab" onclick="showTab('detection')">检测记录</button>
    <button class="tab" onclick="showTab('badges')">工牌信息</button>
    <button class="tab" onclick="showTab('config')">工牌配置</button>
  </div>
  <div class="content">
    <!-- 打卡记录 -->
    <div id="records" class="section active">
      <div id="recordList"></div>
    </div>
    <!-- 检测记录 -->
    <div id="detection" class="section">
      <div id="detectionList"></div>
    </div>
    <!-- 工牌信息 -->
    <div id="badges" class="section">
      <div id="badgeConfigList"></div>
    </div>
    <!-- 工牌配置 -->
    <div id="config" class="section">
      <div class="form-group">
        <label>选择设备</label>
        <div id="deviceList" class="device-list">
          <div class="empty">扫描中...</div>
        </div>
      </div>
      <div class="form-group">
        <label>姓名</label>
        <input type="text" id="name" placeholder="请输入姓名" maxlength="10">
      </div>
      <div class="form-group">
        <label>部门</label>
        <input type="text" id="dept" placeholder="请输入部门" maxlength="20">
      </div>
      <div class="form-group">
        <label>职位</label>
        <input type="text" id="title" placeholder="请输入职位" maxlength="20">
      </div>
      <div class="form-group">
        <label>工号</label>
        <input type="text" id="badgeId" placeholder="请输入工号" maxlength="10">
      </div>
      <button class="btn" onclick="sendConfig()">发送配置到设备</button>
      <div id="statusMsg" class="status-msg"></div>
    </div>
  </div>
</div>

<script>
let selectedDevice = null;

function showTab(tab) {
  document.querySelectorAll('.tab').forEach(t => t.classList.remove('active'));
  document.querySelectorAll('.section').forEach(s => s.classList.remove('active'));
  event.target.classList.add('active');
  document.getElementById(tab).classList.add('active');
  if(tab === 'records') loadRecords();
  if(tab === 'detection') loadDetection();
  if(tab === 'badges') loadBadgeConfigs();
  if(tab === 'config') loadDevices();
}

function loadRecords() {
  fetch('/api/checkin_list')
    .then(r => r.json())
    .then(data => {
      const list = document.getElementById('recordList');
      if(data.length === 0) {
        list.innerHTML = '<div class="empty">暂无打卡记录</div>';
        return;
      }
      list.innerHTML = data.map(r => `
        <div class="record-item">
          <div class="record-info">
            <h3>${r.name} (${r.dept})</h3>
            <p>${r.title} · 工号:${r.id} · 设备:${r.device_id}</p>
          </div>
          <div class="record-status">
            <div class="time">${r.time}</div>
            <div class="rssi">RSSI: ${r.rssi}dBm</div>
          </div>
        </div>
      `).join('');
    });
}

function loadDetection() {
  fetch('/api/detection_list')
    .then(r => r.json())
    .then(data => {
      const list = document.getElementById('detectionList');
      if(data.length === 0) {
        list.innerHTML = '<div class="empty">暂无蓝牙检测记录</div>';
        return;
      }
      list.innerHTML = data.map(r => `
        <div class="record-item">
          <div class="record-info">
            <h3>${r.name}</h3>
            <p>ID: ${r.id}</p>
          </div>
          <div class="record-status">
            <div class="time">${r.time}</div>
            <div class="rssi">RSSI: ${r.rssi}dBm</div>
          </div>
        </div>
      `).join('');
    });
}

function loadDevices() {
  fetch('/api/badge_list')
    .then(r => r.json())
    .then(data => {
      const list = document.getElementById('deviceList');
      if(data.length === 0) {
        list.innerHTML = '<div class="empty">未发现设备，请确保M5Paper在蓝牙范围内</div>';
        return;
      }
      list.innerHTML = data.map(d => `
        <span class="device-tag" onclick="selectDevice('${d.id}', this)">${d.name}</span>
      `).join('');
    });
}

function selectDevice(id, el) {
  selectedDevice = id;
  document.querySelectorAll('.device-tag').forEach(t => t.classList.remove('selected'));
  el.classList.add('selected');

  // 从数据库加载已有配置填充表单
  fetch('/api/badge_config?deviceId=' + encodeURIComponent(id))
    .then(r => r.json())
    .then(data => {
      if (data.found) {
        document.getElementById('name').value = data.name || '';
        document.getElementById('dept').value = data.dept || '';
        document.getElementById('title').value = data.title || '';
        document.getElementById('badgeId').value = data.badge_id || '';
      } else {
        // 数据库中没有该设备的配置，清空表单
        document.getElementById('name').value = '';
        document.getElementById('dept').value = '';
        document.getElementById('title').value = '';
        document.getElementById('badgeId').value = '';
      }
    })
    .catch(e => {
      // 请求失败时不影响选择操作
    });
}

function sendConfig() {
  if(!selectedDevice) { showStatus('请先选择设备', 'error'); return; }
  const name = document.getElementById('name').value.trim();
  const dept = document.getElementById('dept').value.trim();
  const title = document.getElementById('title').value.trim();
  const id = document.getElementById('badgeId').value.trim();
  if(!name || !dept || !title || !id) { showStatus('请填写完整信息', 'error'); return; }

  fetch('/api/config', {
    method: 'POST',
    headers: {'Content-Type': 'application/json'},
    body: JSON.stringify({deviceId: selectedDevice, name, dept, title, id})
  })
  .then(r => r.json())
  .then(r => {
    showStatus(r.success ? '配置发送成功！' : '配置失败: ' + r.error, r.success ? 'success' : 'error');
  })
  .catch(e => showStatus('请求失败', 'error'));
}

function deleteConfig(deviceId) {
  if(!confirm('确定要删除该工牌配置吗？')) return;
  fetch('/api/delete_config', {
    method: 'POST',
    headers: {'Content-Type': 'application/json'},
    body: JSON.stringify({deviceId: deviceId})
  })
  .then(r => r.json())
  .then(r => {
    showStatus(r.success ? '删除成功' : '删除失败: ' + r.error, r.success ? 'success' : 'error');
    if(r.success) loadBadgeConfigs();
  })
  .catch(e => showStatus('请求失败', 'error'));
}

function showStatus(msg, type) {
  const el = document.getElementById('statusMsg');
  el.textContent = msg;
  el.className = 'status-msg ' + type;
  setTimeout(() => el.className = 'status-msg', 3000);
}

function loadBadgeConfigs() {
  fetch('/api/badge_configs')
    .then(r => r.json())
    .then(data => {
      const list = document.getElementById('badgeConfigList');
      if(data.length === 0) {
        list.innerHTML = '<div class="empty">暂无工牌配置记录</div>';
        return;
      }
      list.innerHTML = data.map(b => `
        <div class="record-item">
          <div class="record-info">
            <h3>${b.name} (${b.dept})</h3>
            <p>${b.title} · 工号:${b.badge_id} · 设备:${b.device_id}</p>
          </div>
          <div class="record-status">
            <div class="time">已配置</div>
            <button class="btn" style="padding:6px 12px;font-size:13px;margin-top:6px;" onclick="deleteConfig('${b.device_id}')">删除</button>
          </div>
        </div>
      `).join('');
    });
}

// 自动刷新
setInterval(() => {
  if(document.getElementById('records').classList.contains('active')) loadRecords();
  if(document.getElementById('detection').classList.contains('active')) loadDetection();
  if(document.getElementById('badges').classList.contains('active')) loadBadgeConfigs();
}, 3000);

// 初始加载
loadRecords();
</script>
</body>
</html>
)rawliteral";

#endif
