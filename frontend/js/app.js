'use strict';
const $ = (s) => document.querySelector(s);
const $$ = (s) => [...document.querySelectorAll(s)];
const state = { students: [], stats: null, editing: null, charts: {}, view: 'dashboard' };
const esc = (s) => String(s).replace(/[&<>"']/g, (c) => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;' }[c]));
const fmt = (n) => Number(n).toLocaleString(undefined, { maximumFractionDigits: 2 });

/* ---------- API ---------- */
function setStatus(ok) {
  const el = $('#apiStatus');
  el.className = 'status ' + (ok ? 'ok' : 'bad');
  el.textContent = ok ? 'Server online' : 'Server offline';
}
async function api(path, opts = {}) {
  let res;
  try {
    res = await fetch('/api' + path, { headers: { 'Content-Type': 'application/json' }, ...opts });
  } catch {
    setStatus(false);
    throw new Error('Cannot reach the C++ server. Make sure ranking_server is running.');
  }
  setStatus(true);
  const data = await res.json().catch(() => ({}));
  if (!res.ok) throw new Error(data.error || `Request failed (${res.status}).`);
  return data;
}
async function refresh() {
  const [students, stats] = await Promise.all([api('/students'), api('/stats')]);
  state.students = students;
  state.stats = stats;
  render();
}
function toast(msg, err = false) {
  const t = document.createElement('div');
  t.className = 'toast' + (err ? ' err' : '');
  t.textContent = msg;
  $('#toasts').append(t);
  setTimeout(() => t.remove(), 3200);
}

/* ---------- Rendering ---------- */
function render() {
  const s = state.stats;
  $('#cTotal').textContent = fmt(s.total);
  $('#cAvg').textContent = fmt(s.average);
  $('#cHigh').textContent = fmt(s.highest);
  $('#cDept').textContent = s.departments.length;
  renderPodium();
  renderFilters();
  renderTable();
  renderCharts();
}
function renderPodium() {
  const top = state.stats.top3;
  if (!top.length) { $('#podium').innerHTML = '<p class="empty">No students yet. Add one to start the ranking.</p>'; return; }
  const order = [top[1], top[0], top[2]].filter(Boolean);
  $('#podium').innerHTML = order.map((t) => `
    <div class="slot r${t.rank}"><div class="n">${t.rank}</div><b>${esc(t.name)}</b>
    <small>${esc(t.id)} &middot; ${esc(t.department)}</small><div class="m">${fmt(t.marks)} <span class="grade g-${gcls(t.grade)}">${t.grade}</span></div></div>`).join('');
}
const gcls = (g) => g.replace('+', 'p');
function renderFilters() {
  const sel = $('#deptFilter'), cur = sel.value;
  const depts = state.stats.departments.map((d) => d.name);
  sel.innerHTML = '<option value="">All departments</option>' + depts.map((d) => `<option>${esc(d)}</option>`).join('');
  sel.value = depts.includes(cur) ? cur : '';
  $('#deptList').innerHTML = depts.map((d) => `<option value="${esc(d)}">`).join('');
}
function visibleRows() {
  const q = $('#search').value.trim().toLowerCase(), d = $('#deptFilter').value, g = $('#gradeFilter').value;
  const [key, dir] = $('#sortBy').value.split(':');
  const rows = state.students.filter((s) =>
    (!q || [s.id, s.name, s.department].some((v) => v.toLowerCase().includes(q))) && (!d || s.department === d) && (!g || s.grade === g));
  rows.sort((a, b) => {
    const c = typeof a[key] === 'number' ? a[key] - b[key] : String(a[key]).localeCompare(String(b[key]));
    return (dir === 'desc' ? -c : c) || a.rank - b.rank;
  });
  return rows;
}
function renderTable() {
  const rows = visibleRows();
  $('#tbody').innerHTML = rows.map((s) => `
    <tr><td><span class="rank ${s.rank <= 3 ? 'top' : ''}">${s.rank}</span></td>
    <td class="name">${esc(s.name)}</td><td class="muted">${esc(s.id)}</td><td>${esc(s.department)}</td>
    <td>${fmt(s.marks)}<div class="bar"><i style="width:${s.marks}%"></i></div></td>
    <td><span class="grade g-${gcls(s.grade)}">${s.grade}</span></td>
    <td class="right"><button class="link" data-act="edit" data-id="${esc(s.id)}">Edit</button><button class="link del" data-act="del" data-id="${esc(s.id)}">Delete</button></td></tr>`).join('');
  $('#empty').hidden = rows.length > 0;
  $('#count').textContent = `Showing ${rows.length} of ${state.students.length} students`;
}

/* ---------- Charts ---------- */
const css = (v) => getComputedStyle(document.documentElement).getPropertyValue(v).trim();
function mk(id, cfg) {
  if (state.charts[id]) state.charts[id].destroy();
  const el = $('#' + id);
  if (!el || !el.offsetParent) return;  // canvas is in a hidden view
  state.charts[id] = new Chart(el, cfg);
}
function renderCharts() {
  Chart.defaults.color = css('--muted');
  Chart.defaults.borderColor = css('--line');
  Chart.defaults.font.family = '"Segoe UI",system-ui,sans-serif';
  const accent = css('--accent'), gold = css('--gold'), s = state.stats;
  const base = { responsive: true, maintainAspectRatio: false, plugins: { legend: { display: false } } };
  const top10 = state.students.slice(0, 10);
  mk('chTop', { type: 'bar', data: { labels: top10.map((t) => t.name), datasets: [{ data: top10.map((t) => t.marks), backgroundColor: top10.map((t) => (t.rank === 1 ? gold : accent)), borderRadius: 4 }] },
    options: { ...base, indexAxis: 'y', scales: { x: { min: 0, max: 100 } } } });
  mk('chDist', { type: 'bar', data: { labels: s.distribution.map((d) => d.label), datasets: [{ label: 'Students', data: s.distribution.map((d) => d.count), backgroundColor: accent, borderRadius: 4 }] },
    options: { ...base, scales: { y: { beginAtZero: true, ticks: { precision: 0 } } } } });
  mk('chDept', { type: 'bar', data: { labels: s.departments.map((d) => d.name), datasets: [{ label: 'Average marks', data: s.departments.map((d) => d.average), backgroundColor: gold, borderRadius: 4 }] },
    options: { ...base, scales: { y: { min: 0, max: 100 } } } });
  const gl = Object.keys(s.grades);
  mk('chGrade', { type: 'doughnut', data: { labels: gl, datasets: [{ data: gl.map((g) => s.grades[g]), borderWidth: 0,
      backgroundColor: ['#0f766e', '#2a9d8f', '#8ab17d', '#e9c46a', '#f4a261', '#d1495b'] }] },
    options: { responsive: true, maintainAspectRatio: false, plugins: { legend: { position: 'bottom' } } } });
}

/* ---------- Navigation & theme ---------- */
const titles = { dashboard: 'Dashboard', leaderboard: 'Leaderboard', analytics: 'Analytics' };
function show(view) {
  state.view = view;
  $$('.view').forEach((v) => v.classList.toggle('active', v.id === 'view-' + view));
  $$('.nav').forEach((n) => n.classList.toggle('active', n.dataset.view === view));
  $('#viewTitle').textContent = titles[view];
  $('#sidebar').classList.remove('open');
  if (state.stats) renderCharts();
}
function setTheme(t) {
  document.documentElement.dataset.theme = t;
  localStorage.setItem('theme', t);
  if (state.stats) renderCharts();
}
$$('.nav').forEach((n) => n.addEventListener('click', () => show(n.dataset.view)));
$('#menuBtn').onclick = () => $('#sidebar').classList.toggle('open');
$('#scrim').onclick = () => $('#sidebar').classList.remove('open');
$('#themeBtn').onclick = () => setTheme(document.documentElement.dataset.theme === 'dark' ? 'light' : 'dark');
['#search', '#deptFilter', '#gradeFilter', '#sortBy'].forEach((s) => $(s).addEventListener('input', renderTable));

/* ---------- Add / edit / delete ---------- */
function openForm(student) {
  state.editing = student ? student.id : null;
  $('#formTitle').textContent = student ? 'Edit student' : 'Add student';
  $('#fId').value = student ? student.id : '';
  $('#fId').disabled = !!student;
  $('#fName').value = student ? student.name : '';
  $('#fDept').value = student ? student.department : '';
  $('#fMarks').value = student ? student.marks : '';
  $('#formError').textContent = '';
  $('#formDlg').showModal();
  (student ? $('#fName') : $('#fId')).focus();
}
$('#addBtn').onclick = () => openForm(null);
$('#cancelBtn').onclick = () => $('#formDlg').close();
$('#studentForm').addEventListener('submit', async (e) => {
  e.preventDefault();
  const body = { id: $('#fId').value, name: $('#fName').value, department: $('#fDept').value, marks: parseFloat($('#fMarks').value) };
  if (!body.id.trim() || !body.name.trim() || !body.department.trim() || Number.isNaN(body.marks)) {
    $('#formError').textContent = 'Fill in every field, including marks.'; return;
  }
  try {
    if (state.editing) await api('/students/' + encodeURIComponent(state.editing), { method: 'PUT', body: JSON.stringify(body) });
    else await api('/students', { method: 'POST', body: JSON.stringify(body) });
    $('#formDlg').close();
    toast(state.editing ? 'Student updated' : 'Student added');
    await refresh();
  } catch (err) { $('#formError').textContent = err.message; }
});
$('#tbody').addEventListener('click', (e) => {
  const b = e.target.closest('button[data-act]');
  if (!b) return;
  const s = state.students.find((x) => x.id === b.dataset.id);
  if (b.dataset.act === 'edit') return openForm(s);
  $('#confirmMsg').textContent = `${s.name} (${s.id}) will be removed from the ranking.`;
  $('#confirmDlg').dataset.id = s.id;
  $('#confirmDlg').showModal();
});
$('#noBtn').onclick = () => $('#confirmDlg').close();
$('#yesBtn').onclick = async () => {
  const id = $('#confirmDlg').dataset.id;
  $('#confirmDlg').close();
  try { await api('/students/' + encodeURIComponent(id), { method: 'DELETE' }); toast('Student deleted'); await refresh(); }
  catch (err) { toast(err.message, true); }
};

/* ---------- Boot ---------- */
setTheme(localStorage.getItem('theme') || (matchMedia('(prefers-color-scheme: dark)').matches ? 'dark' : 'light'));
refresh().catch((err) => toast(err.message, true));
