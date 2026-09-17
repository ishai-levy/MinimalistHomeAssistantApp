import { useState, useEffect } from "react"

// ─── Types ────────────────────────────────────────────────────────────────────

type Tab = "clock" | "home" | "music"

interface DayForecast  { day: string; icon: string; high: number; low: number }
interface HourForecast { time: string; icon: string; temp: number }

interface TrackInfo {
  title: string; artist: string; album: string
  duration: number; position: number; playing: boolean
  volume: number; favorited: boolean; shuffle: boolean
}

interface QueueItem { id: string; title: string; artist: string; duration: number }

interface Player       { player_id: string; name: string }
interface Playlist     { name: string; uri: string }
interface ScheduleEntry { id: string; kind: "alarm" | "timer"; trigger_at: number; player_id: string }
interface RingingEntry  { id: string; kind: "alarm" | "timer"; player_id: string }

// ─── Mock data ────────────────────────────────────────────────────────────────

const MOCK_DAILY: DayForecast[] = [
  { day: "Mon", icon: "🌙",  high: 25.9, low: 25.9 },
  { day: "Tue", icon: "🌤️", high: 31.4, low: 21.7 },
  { day: "Wed", icon: "🌤️", high: 31.4, low: 21.8 },
  { day: "Thu", icon: "🌤️", high: 30.6, low: 21.3 },
  { day: "Fri", icon: "🌤️", high: 28.9, low: 24.1 },
  { day: "Sat", icon: "🌤️", high: 28.7, low: 24.4 },
]

const MOCK_HOURLY: HourForecast[] = [
  { time: "12 AM", icon: "🌙", temp: 25.3 },
  { time: "1 AM",  icon: "🌙", temp: 24.6 },
  { time: "2 AM",  icon: "🌙", temp: 23.8 },
  { time: "3 AM",  icon: "🌙", temp: 23.1 },
  { time: "4 AM",  icon: "☁️",  temp: 22.8 },
  { time: "5 AM",  icon: "☁️",  temp: 22.3 },
  { time: "6 AM",  icon: "☁️",  temp: 21.7 },
  { time: "7 AM",  icon: "🌤️", temp: 22.4 },
]

const MOCK_TRACK: TrackInfo = {
  title: "Breathe (In the Air)", artist: "Pink Floyd",
  album: "The Dark Side of the Moon",
  duration: 163, position: 47, playing: true,
  volume: 72, favorited: false, shuffle: false,
}

const MOCK_QUEUE: QueueItem[] = [
  { id: "1", title: "Time",                     artist: "Pink Floyd", duration: 421 },
  { id: "2", title: "The Great Gig in the Sky", artist: "Pink Floyd", duration: 284 },
  { id: "3", title: "Money",                    artist: "Pink Floyd", duration: 382 },
  { id: "4", title: "Us and Them",              artist: "Pink Floyd", duration: 469 },
  { id: "5", title: "Any Colour You Like",      artist: "Pink Floyd", duration: 213 },
]

const PLAYERS: Player[] = [
  { player_id: "home_group", name: "Home group" },
  { player_id: "kitchen",    name: "Kitchen"    },
  { player_id: "bedroom",    name: "Bedroom"    },
]

const PLAYLISTS: Playlist[] = [
  { name: "Chill Vibes",    uri: "library://playlist/1" },
  { name: "Rock Classics",  uri: "library://playlist/2" },
  { name: "Morning Coffee", uri: "library://playlist/3" },
  { name: "Late Night",     uri: "library://playlist/4" },
  { name: "Party Mix",      uri: "library://playlist/5" },
  { name: "Focus Mode",     uri: "library://playlist/6" },
]

// ─── Helpers ──────────────────────────────────────────────────────────────────

const pad = (n: number) => String(n).padStart(2, "0")
const fmtDur = (s: number) => `${Math.floor(s / 60)}:${pad(s % 60)}`
const DAYS   = ["Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"]
const MONTHS = ["Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"]

function fmtAlarmTime(trigger_at: number): string {
  const d = new Date(trigger_at * 1000)
  const h = d.getHours(), m = d.getMinutes()
  const ampm = h >= 12 ? "PM" : "AM"
  return `${pad(h % 12 || 12)}:${pad(m)} ${ampm}`
}

let _uid = 0
const uid = () => `${Date.now()}-${++_uid}`

// ─── Color palette ────────────────────────────────────────────────────────────

const C = {
  accent:  { bg: "rgba(0,200,240,0.13)",  border: "var(--color-accent)",   text: "var(--color-accent)"   },
  warm:    { bg: "rgba(255,112,67,0.13)", border: "var(--color-warm)",     text: "var(--color-warm)"     },
  success: { bg: "rgba(38,208,124,0.13)", border: "var(--color-success)",  text: "var(--color-success)"  },
  neutral: { bg: "var(--color-surface-2)",border: "var(--color-border)",   text: "var(--color-text)"     },
} as const
type ColorKey = keyof typeof C

// ─── PlayerPicker ─────────────────────────────────────────────────────────────

function PlayerPicker({ value, onChange }: { value: string | null; onChange: (id: string) => void }) {
  const [open, setOpen] = useState(false)
  const selected = PLAYERS.find(p => p.player_id === value)

  return (
    <div style={{ position: "relative", display: "flex", alignItems: "center", gap: 8 }}>
      <span style={{ fontSize: 10, color: "var(--color-text-3)", fontFamily: "var(--font-mono)", letterSpacing: 1.5, textTransform: "uppercase", flexShrink: 0 }}>Player</span>
      <button
        onClick={() => setOpen(o => !o)}
        style={{
          display: "flex", alignItems: "center", gap: 7,
          padding: "5px 10px 5px 12px", borderRadius: 8,
          background: open ? C.accent.bg : "var(--color-surface-3)",
          border: `1px solid ${open ? C.accent.border : "var(--color-border)"}`,
          color: open ? C.accent.text : "var(--color-text-2)",
          fontSize: 12, fontFamily: "var(--font-ui)", fontWeight: 500,
          transition: "all 0.12s", whiteSpace: "nowrap",
        }}
      >
        {selected?.name ?? "—"}
        <span style={{ fontSize: 9, opacity: 0.7, marginLeft: 2 }}>{open ? "▲" : "▼"}</span>
      </button>

      {open && (
        <>
          {/* backdrop to close on outside click */}
          <div
            onClick={() => setOpen(false)}
            style={{ position: "fixed", inset: 0, zIndex: 299 }}
          />
          <div style={{
            position: "absolute", bottom: "calc(100% + 6px)", left: 0,
            zIndex: 300,
            background: "var(--color-surface)",
            border: "1px solid var(--color-border-bright)",
            borderRadius: 12,
            minWidth: 180, maxHeight: 220, overflowY: "auto",
            boxShadow: "0 8px 32px rgba(0,0,0,0.5)",
            display: "flex", flexDirection: "column", gap: 2, padding: 6,
          }}>
            {PLAYERS.map(p => {
              const sel = value === p.player_id
              return (
                <button
                  key={p.player_id}
                  onClick={() => { onChange(p.player_id); setOpen(false) }}
                  style={{
                    padding: "9px 14px", borderRadius: 8, textAlign: "left",
                    background: sel ? C.accent.bg : "transparent",
                    border: `1px solid ${sel ? C.accent.border : "transparent"}`,
                    color: sel ? C.accent.text : "var(--color-text-2)",
                    fontSize: 13, fontFamily: "var(--font-ui)", fontWeight: sel ? 600 : 400,
                    transition: "all 0.1s", whiteSpace: "nowrap",
                  }}
                >{p.name}</button>
              )
            })}
          </div>
        </>
      )}
    </div>
  )
}

// ─── Tab Bar ──────────────────────────────────────────────────────────────────

const TABS: { id: Tab; label: string; icon: string }[] = [
  { id: "clock", label: "Clock & Weather", icon: "🕐" },
  { id: "home",  label: "Home",            icon: "🏠" },
  { id: "music", label: "Music",           icon: "🎵" },
]

function TabBar({ active, onChange }: { active: Tab; onChange: (t: Tab) => void }) {
  return (
    <div style={{
      display: "flex", gap: 4,
      background: "var(--color-surface-2)",
      border: "1px solid var(--color-border)",
      borderRadius: 14, padding: 4,
    }}>
      {TABS.map(t => (
        <button key={t.id} onClick={() => onChange(t.id)} style={{
          display: "flex", alignItems: "center", gap: 8,
          padding: "9px 20px", borderRadius: 10,
          background: active === t.id ? "var(--color-accent)" : "transparent",
          border: "none",
          color: active === t.id ? "var(--color-background)" : "var(--color-text-2)",
          fontSize: 14, fontWeight: active === t.id ? 600 : 400, fontFamily: "var(--font-ui)",
          transition: "all 0.15s ease", whiteSpace: "nowrap",
        }}>
          <span style={{ fontSize: 15 }}>{t.icon}</span>
          {t.label}
        </button>
      ))}
    </div>
  )
}

// ─── Tab 1: Clock + Weather ───────────────────────────────────────────────────

function ClockWeatherTab() {
  const [now, setNow] = useState(new Date())

  useEffect(() => {
    const t = setInterval(() => setNow(new Date()), 1000)
    return () => clearInterval(t)
  }, [])

  const h = now.getHours()
  const ampm = h >= 12 ? "PM" : "AM"
  const h12  = h % 12 || 12

  return (
    <div style={{ display: "flex", width: "100%", height: "100%", gap: 0 }}>
      <div style={{
        width: "50%", flexShrink: 0,
        display: "flex", flexDirection: "column", justifyContent: "center",
        padding: "0 56px", borderRight: "1px solid var(--color-border)",
      }}>
        <div style={{
          fontFamily: "var(--font-mono)", fontSize: 140, fontWeight: 300,
          lineHeight: 1, color: "var(--color-text)", letterSpacing: "-6px",
          display: "flex", alignItems: "baseline", gap: 12,
        }}>
          <span>{pad(h12)}:{pad(now.getMinutes())}</span>
          <span style={{ fontSize: 34, fontWeight: 400, color: "var(--color-accent)", letterSpacing: 0, marginBottom: 10 }}>{ampm}</span>
        </div>
        <div style={{ fontFamily: "var(--font-mono)", fontSize: 18, color: "var(--color-text-2)", letterSpacing: 3, textTransform: "uppercase", marginTop: 14 }}>
          {DAYS[now.getDay()]}
        </div>
        <div style={{ fontFamily: "var(--font-mono)", fontSize: 16, color: "var(--color-text-3)", letterSpacing: 2, textTransform: "uppercase", marginTop: 5 }}>
          {MONTHS[now.getMonth()]} {now.getDate()}, {now.getFullYear()}
        </div>

        <div style={{ height: 1, background: "var(--color-border)", margin: "28px 0" }} />

        <div style={{ display: "flex", alignItems: "center", gap: 16, marginBottom: 24 }}>
          <span style={{ fontSize: 56 }}>🌙</span>
          <div>
            <div style={{ fontFamily: "var(--font-mono)", fontSize: 48, fontWeight: 300, lineHeight: 1 }}>25.9°</div>
            <div style={{ fontSize: 13, color: "var(--color-text-2)", marginTop: 4 }}>Clear, night · 4 hours ago</div>
          </div>
        </div>

        <div style={{ display: "grid", gridTemplateColumns: "1fr 1fr", gap: 8 }}>
          {[
            { label: "Humidity",   value: "84%"          },
            { label: "Wind",       value: "9.4 km/h NE"  },
            { label: "Pressure",   value: "1,014.1 hPa"  },
            { label: "Feels like", value: "25.9°"        },
          ].map(({ label, value }) => (
            <div key={label} style={{
              background: "var(--color-surface-2)", borderRadius: 8,
              padding: "10px 14px", border: "1px solid var(--color-border)",
            }}>
              <div style={{ fontSize: 10, color: "var(--color-text-3)", textTransform: "uppercase", letterSpacing: 1.5, marginBottom: 3, fontFamily: "var(--font-mono)" }}>{label}</div>
              <div style={{ fontSize: 15, fontWeight: 500, fontFamily: "var(--font-mono)" }}>{value}</div>
            </div>
          ))}
        </div>
      </div>

      <div style={{ flex: 1, display: "flex", flexDirection: "column", padding: "24px 36px", justifyContent: "center", gap: 28 }}>
        <div>
          <div style={{ fontSize: 10, letterSpacing: 2.5, textTransform: "uppercase", color: "var(--color-text-3)", fontFamily: "var(--font-mono)", marginBottom: 12 }}>Daily</div>
          <div style={{ display: "grid", gridTemplateColumns: "repeat(3, 1fr)", gap: 12 }}>
            {MOCK_DAILY.slice(0, 3).map(d => (
              <div key={d.day} style={{
                background: "var(--color-surface-2)", border: "1px solid var(--color-border)",
                borderRadius: 12, padding: "14px 8px",
                display: "flex", flexDirection: "column", alignItems: "center", gap: 8,
              }}>
                <div style={{ fontSize: 11, color: "var(--color-text-2)", fontFamily: "var(--font-mono)", letterSpacing: 1 }}>{d.day}</div>
                <span style={{ fontSize: 26 }}>{d.icon}</span>
                <div style={{ fontSize: 14, fontWeight: 500, fontFamily: "var(--font-mono)" }}>{d.high}°</div>
                <div style={{ fontSize: 12, color: "var(--color-text-3)", fontFamily: "var(--font-mono)" }}>{d.low}°</div>
              </div>
            ))}
          </div>
        </div>

        <div>
          <div style={{ fontSize: 10, letterSpacing: 2.5, textTransform: "uppercase", color: "var(--color-text-3)", fontFamily: "var(--font-mono)", marginBottom: 12 }}>Hourly</div>
          <div style={{ display: "grid", gridTemplateColumns: "repeat(3, 1fr)", gap: 12 }}>
            {MOCK_HOURLY.slice(0, 3).map(hr => (
              <div key={hr.time} style={{
                background: "var(--color-surface-2)", border: "1px solid var(--color-border)",
                borderRadius: 12, padding: "12px 6px",
                display: "flex", flexDirection: "column", alignItems: "center", gap: 8,
              }}>
                <div style={{ fontSize: 10, color: "var(--color-text-2)", fontFamily: "var(--font-mono)", textAlign: "center", lineHeight: 1.3 }}>{hr.time}</div>
                <span style={{ fontSize: 22 }}>{hr.icon}</span>
                <div style={{ fontSize: 13, fontWeight: 500, fontFamily: "var(--font-mono)" }}>{hr.temp}°</div>
              </div>
            ))}
          </div>
        </div>
      </div>
    </div>
  )
}

// ─── Tab 2: Home Controls ─────────────────────────────────────────────────────

function SimpleCard({ icon, label, color, toggle, isOn, isSent, onPress }: {
  icon: string; label: string; color: ColorKey
  toggle: boolean; isOn: boolean; isSent: boolean; onPress: () => void
}) {
  const active = toggle ? isOn : isSent
  return (
    <button onClick={onPress} style={{
      background: active ? C[color].bg : "var(--color-surface-2)",
      border: `1px solid ${active ? C[color].border : "var(--color-border)"}`,
      borderRadius: 16, padding: "22px 24px",
      display: "flex", flexDirection: "column", justifyContent: "space-between", alignItems: "flex-start",
      transition: "all 0.15s ease",
      boxShadow: active ? `0 0 22px ${C[color].bg}` : "none",
      height: "100%", width: "100%",
    }}>
      <span style={{ fontSize: 42 }}>{icon}</span>
      <div>
        <div style={{ fontSize: 18, fontWeight: 600, color: active ? C[color].text : "var(--color-text)", textAlign: "left" }}>{label}</div>
        {toggle && (
          <div style={{ fontSize: 11, fontFamily: "var(--font-mono)", color: active ? C[color].text : "var(--color-text-3)", marginTop: 3, textTransform: "uppercase", letterSpacing: 1.5 }}>
            {isOn ? "ON" : "OFF"}
          </div>
        )}
        {!toggle && isSent && (
          <div style={{ fontSize: 11, fontFamily: "var(--font-mono)", color: C[color].text, marginTop: 3 }}>sent…</div>
        )}
      </div>
    </button>
  )
}

const ROBO_BTNS: { cmd: string; label: string; icon: string; col: ColorKey; desc: string }[] = [
  { cmd: "vacuum_start", label: "Start",    icon: "▶",  col: "success", desc: "Begin cleaning cycle" },
  { cmd: "vacuum_stop",  label: "Stop",     icon: "⏹",  col: "warm",    desc: "Pause and stay put"   },
  { cmd: "vacuum_home",  label: "Dock",     icon: "🏠", col: "neutral", desc: "Return to base"       },
]

function RoboModal({ vacuumState, onClose }: { vacuumState: string; onClose: () => void }) {
  const [sent, setSent] = useState<string | null>(null)
  const running = vacuumState === "cleaning"
  const docked  = vacuumState === "docked" || vacuumState === "charging"

  function fire(cmd: string) {
    setSent(cmd)
    setTimeout(() => setSent(null), 1800)
    // TODO: mqtt publish dashboard/cmd/<cmd>
  }

  return (
    <div
      onClick={e => { if (e.target === e.currentTarget) onClose() }}
      style={{
        position: "absolute", inset: 0,
        background: "rgba(0,0,0,0.72)",
        display: "flex", alignItems: "center", justifyContent: "center",
        zIndex: 100,
      }}
    >
      <div style={{
        background: "var(--color-surface)",
        border: "1px solid var(--color-border-bright)",
        borderRadius: 22, padding: "30px 36px",
        width: 380, display: "flex", flexDirection: "column", gap: 24,
      }}>
        {/* Header */}
        <div style={{ display: "flex", alignItems: "center", justifyContent: "space-between" }}>
          <div style={{ display: "flex", alignItems: "center", gap: 12 }}>
            <span style={{ fontSize: 30 }}>🤖</span>
            <div>
              <div style={{ fontSize: 17, fontWeight: 700 }}>Roborock</div>
              <div style={{ fontSize: 11, fontFamily: "var(--font-mono)", marginTop: 2,
                color: running ? C.success.text : docked ? "var(--color-text-3)" : "var(--color-text-2)" }}>
                {running ? "● cleaning" : docked ? "○ docked" : `○ ${vacuumState || "idle"}`}
              </div>
            </div>
          </div>
          <button onClick={onClose} style={{
            width: 34, height: 34, borderRadius: 8,
            background: "var(--color-surface-3)", border: "1px solid var(--color-border)",
            color: "var(--color-text-2)", fontSize: 15,
            display: "flex", alignItems: "center", justifyContent: "center",
          }}>✕</button>
        </div>

        {/* Controls */}
        <div style={{ display: "flex", flexDirection: "column", gap: 10 }}>
          {ROBO_BTNS.map(b => {
            const active = sent === b.cmd
            return (
              <button key={b.cmd} onClick={() => fire(b.cmd)} style={{
                padding: "16px 20px", borderRadius: 12,
                background: active ? C[b.col].bg : "var(--color-surface-2)",
                border: `1px solid ${active ? C[b.col].border : "var(--color-border)"}`,
                color: active ? C[b.col].text : "var(--color-text)",
                display: "flex", alignItems: "center", gap: 16,
                transition: "all 0.15s",
                boxShadow: active ? `0 0 18px ${C[b.col].bg}` : "none",
              }}>
                <span style={{ fontSize: 22, width: 28, textAlign: "center" }}>{b.icon}</span>
                <div style={{ textAlign: "left" }}>
                  <div style={{ fontSize: 15, fontWeight: 600, fontFamily: "var(--font-ui)" }}>{b.label}</div>
                  <div style={{ fontSize: 11, color: active ? C[b.col].text : "var(--color-text-3)", marginTop: 2, fontFamily: "var(--font-mono)" }}>{b.desc}</div>
                </div>
              </button>
            )
          })}
        </div>
      </div>
    </div>
  )
}

function RoboCard({ vacuumState }: { vacuumState: string }) {
  const [modalOpen, setModalOpen] = useState(false)
  const running = vacuumState === "cleaning"
  const docked  = vacuumState === "docked" || vacuumState === "charging"

  return (
    <>
      <button onClick={() => setModalOpen(true)} style={{
        background: running ? C.success.bg : "var(--color-surface-2)",
        border: `1px solid ${running ? C.success.border : "var(--color-border)"}`,
        borderRadius: 16, padding: "22px 24px",
        display: "flex", flexDirection: "column", justifyContent: "space-between", alignItems: "flex-start",
        transition: "all 0.2s ease",
        boxShadow: running ? `0 0 26px ${C.success.bg}` : "none",
        height: "100%", width: "100%",
      }}>
        <span style={{ fontSize: 42 }}>🤖</span>
        <div>
          <div style={{ fontSize: 18, fontWeight: 600, color: running ? C.success.text : "var(--color-text)" }}>Roborock</div>
          <div style={{ fontSize: 11, fontFamily: "var(--font-mono)", marginTop: 3,
            color: running ? C.success.text : docked ? "var(--color-text-3)" : "var(--color-text-2)",
            textTransform: "uppercase", letterSpacing: 1.5 }}>
            {running ? "● CLEANING" : docked ? "○ DOCKED" : `○ ${(vacuumState || "IDLE").toUpperCase()}`}
          </div>
        </div>
      </button>
      {modalOpen && <RoboModal vacuumState={vacuumState} onClose={() => setModalOpen(false)} />}
    </>
  )
}

// ─── AlarmCard ────────────────────────────────────────────────────────────────

function AlarmCard({ alarms, onOpen }: { alarms: ScheduleEntry[]; onOpen: () => void }) {
  const next = alarms[0]
  const active = alarms.length > 0
  return (
    <button onClick={onOpen} style={{
      background: active ? C.accent.bg : "var(--color-surface-2)",
      border: `1px solid ${active ? C.accent.border : "var(--color-border)"}`,
      borderRadius: 16, padding: "22px 24px",
      display: "flex", flexDirection: "column", justifyContent: "space-between", alignItems: "flex-start",
      height: "100%", width: "100%",
      transition: "all 0.15s ease",
      boxShadow: active ? `0 0 22px ${C.accent.bg}` : "none",
    }}>
      <div style={{ display: "flex", alignItems: "flex-start", justifyContent: "space-between", width: "100%" }}>
        <span style={{ fontSize: 42 }}>⏰</span>
        {alarms.length > 0 && (
          <span style={{ fontSize: 11, fontFamily: "var(--font-mono)", color: C.accent.text,
            background: C.accent.bg, border: `1px solid ${C.accent.border}`,
            borderRadius: 5, padding: "2px 7px", marginTop: 4 }}>
            {alarms.length}
          </span>
        )}
      </div>
      <div>
        <div style={{ fontSize: 18, fontWeight: 600, color: active ? C.accent.text : "var(--color-text)", textAlign: "left" }}>
          Set Alarm
        </div>
        <div style={{ fontSize: 11, fontFamily: "var(--font-mono)", color: active ? C.accent.text : "var(--color-text-3)", marginTop: 3, textTransform: "uppercase", letterSpacing: 1.5 }}>
          {next ? `→ ${fmtAlarmTime(next.trigger_at)}` : "NONE SET"}
        </div>
      </div>
    </button>
  )
}

// ─── SpinnerField ─────────────────────────────────────────────────────────────

function SpinnerField({ value, min, max, onChange }: { value: number; min: number; max: number; onChange: (v: number) => void }) {
  const inc = () => onChange(value >= max ? min : value + 1)
  const dec = () => onChange(value <= min ? max : value - 1)
  const btn: React.CSSProperties = {
    width: 44, height: 44, borderRadius: 10,
    background: "var(--color-surface-3)", border: "1px solid var(--color-border-bright)",
    color: "var(--color-text-2)", fontSize: 17,
    display: "flex", alignItems: "center", justifyContent: "center",
    transition: "all 0.12s",
  }
  return (
    <div style={{ display: "flex", flexDirection: "column", alignItems: "center", gap: 10 }}>
      <button onClick={inc} style={btn}>▲</button>
      <div style={{ fontFamily: "var(--font-mono)", fontSize: 58, fontWeight: 300, width: 88, textAlign: "center", lineHeight: 1, color: "var(--color-text)" }}>{pad(value)}</div>
      <button onClick={dec} style={btn}>▼</button>
    </div>
  )
}

// ─── AlarmModal ───────────────────────────────────────────────────────────────

function AlarmModal({ alarms, onClose, onSet, onCancel }: {
  alarms: ScheduleEntry[]
  onClose: () => void
  onSet: (hh: number, mm: number, player_id: string) => void
  onCancel: (id: string) => void
}) {
  const now = new Date()
  const [hh, setHh] = useState(now.getHours())
  const [mm, setMm] = useState(0)
  const [playerId, setPlayerId] = useState(PLAYERS[0].player_id)

  return (
    <div
      onClick={e => { if (e.target === e.currentTarget) onClose() }}
      style={{
        position: "absolute", inset: 0,
        background: "rgba(0,0,0,0.72)",
        display: "flex", alignItems: "center", justifyContent: "center",
        zIndex: 100,
      }}
    >
      <div style={{
        background: "var(--color-surface)",
        border: "1px solid var(--color-border-bright)",
        borderRadius: 22, padding: "34px 40px",
        width: 460, display: "flex", flexDirection: "column", gap: 24,
        maxHeight: 680, overflowY: "auto",
      }}>
        {/* Header */}
        <div style={{ display: "flex", alignItems: "center", justifyContent: "space-between" }}>
          <div style={{ fontSize: 17, fontWeight: 700, letterSpacing: 0.5 }}>⏰ Set Alarm</div>
          <button onClick={onClose} style={{
            width: 34, height: 34, borderRadius: 8,
            background: "var(--color-surface-3)", border: "1px solid var(--color-border)",
            color: "var(--color-text-2)", fontSize: 15,
            display: "flex", alignItems: "center", justifyContent: "center",
          }}>✕</button>
        </div>

        {/* Time picker */}
        <div style={{ display: "flex", alignItems: "center", justifyContent: "center", gap: 14 }}>
          <SpinnerField value={hh} min={0} max={23} onChange={setHh} />
          <span style={{ fontFamily: "var(--font-mono)", fontSize: 50, fontWeight: 300, color: "var(--color-accent)", paddingBottom: 4 }}>:</span>
          <SpinnerField value={mm} min={0} max={59} onChange={setMm} />
        </div>

        {/* Player */}
        <PlayerPicker value={playerId} onChange={setPlayerId} />

        {/* Confirm */}
        <button
          onClick={() => { onSet(hh, mm, playerId); onClose() }}
          style={{
            padding: "14px", borderRadius: 12,
            background: "var(--color-accent)", border: "none",
            color: "var(--color-background)", fontSize: 15, fontWeight: 700,
            fontFamily: "var(--font-ui)", letterSpacing: 1,
          }}
        >
          Set for {pad(hh)}:{pad(mm)}
        </button>

        {/* Upcoming alarms */}
        {alarms.length > 0 && (
          <>
            <div style={{ height: 1, background: "var(--color-border)" }} />
            <div>
              <div style={{ fontSize: 10, letterSpacing: 2.5, textTransform: "uppercase", color: "var(--color-text-3)", fontFamily: "var(--font-mono)", marginBottom: 12 }}>Upcoming</div>
              <div style={{ display: "flex", flexDirection: "column", gap: 8 }}>
                {alarms.map(a => {
                  const player = PLAYERS.find(p => p.player_id === a.player_id)
                  return (
                    <div key={a.id} style={{
                      display: "flex", alignItems: "center", justifyContent: "space-between",
                      background: "var(--color-surface-2)", border: "1px solid var(--color-border)",
                      borderRadius: 10, padding: "10px 14px",
                    }}>
                      <div>
                        <div style={{ fontFamily: "var(--font-mono)", fontSize: 20, fontWeight: 300 }}>{fmtAlarmTime(a.trigger_at)}</div>
                        <div style={{ fontSize: 11, color: "var(--color-text-3)", marginTop: 2 }}>{player?.name ?? a.player_id}</div>
                      </div>
                      <button onClick={() => onCancel(a.id)} style={{
                        width: 32, height: 32, borderRadius: 7,
                        background: C.warm.bg, border: `1px solid ${C.warm.border}`,
                        color: C.warm.text, fontSize: 14,
                        display: "flex", alignItems: "center", justifyContent: "center",
                      }}>✕</button>
                    </div>
                  )
                })}
              </div>
            </div>
          </>
        )}
      </div>
    </div>
  )
}

// ─── TimerKeypadModal ─────────────────────────────────────────────────────────

function TimerKeypadModal({ timers, onClose, onStart, onCancelTimer }: {
  timers: ScheduleEntry[]
  onClose: () => void
  onStart: (seconds: number, player_id: string) => void
  onCancelTimer: (id: string) => void
}) {
  const [digits, setDigits] = useState<number[]>([])
  const [playerId, setPlayerId] = useState(PLAYERS[0].player_id)

  // digits are filled right-to-left: last pressed = rightmost
  // padded to [MM_tens, MM_ones, SS_tens, SS_ones]
  const full = [...Array(Math.max(0, 4 - digits.length)).fill(0), ...digits]
  const mm   = full[0] * 10 + full[1]
  const ss   = full[2] * 10 + full[3]
  const total = mm * 60 + ss
  const valid = total > 0 && ss < 60

  function push(d: number) {
    if (digits.length >= 4) return
    // when this digit becomes the tens-of-seconds position, reject > 5
    if (digits.length === 2 && d > 5) return
    setDigits(prev => [...prev, d])
  }

  function del() { setDigits(prev => prev.slice(0, -1)) }

  const displayStr = `${pad(mm)}:${pad(ss)}`
  const invalid    = digits.length > 0 && !valid

  const keypad = [[1, 2, 3], [4, 5, 6], [7, 8, 9], [null, 0, "⌫"]] as const

  const numBtnStyle = (k: number | null | "⌫"): React.CSSProperties => ({
    height: 62, borderRadius: 12,
    background: k === "⌫" ? "var(--color-surface-3)" : "var(--color-surface-2)",
    border: `1px solid var(--color-border)`,
    color: k === "⌫" ? "var(--color-warm)" : "var(--color-text)",
    fontSize: k === "⌫" ? 20 : 22,
    fontWeight: 400,
    fontFamily: k === "⌫" ? "var(--font-ui)" : "var(--font-mono)",
    display: "flex", alignItems: "center", justifyContent: "center",
    transition: "all 0.1s",
  })

  return (
    <div
      onClick={e => { if (e.target === e.currentTarget) onClose() }}
      style={{
        position: "absolute", inset: 0,
        background: "rgba(0,0,0,0.72)",
        display: "flex", alignItems: "center", justifyContent: "center",
        zIndex: 100,
      }}
    >
      <div style={{
        background: "var(--color-surface)",
        border: "1px solid var(--color-border-bright)",
        borderRadius: 22, padding: "30px 36px",
        width: 380, display: "flex", flexDirection: "column", gap: 20,
        maxHeight: 700, overflowY: "auto",
      }}>
        {/* Header */}
        <div style={{ display: "flex", alignItems: "center", justifyContent: "space-between" }}>
          <div style={{ fontSize: 17, fontWeight: 700, letterSpacing: 0.5 }}>⏱ Set Timer</div>
          <button onClick={onClose} style={{
            width: 34, height: 34, borderRadius: 8,
            background: "var(--color-surface-3)", border: "1px solid var(--color-border)",
            color: "var(--color-text-2)", fontSize: 15,
            display: "flex", alignItems: "center", justifyContent: "center",
          }}>✕</button>
        </div>

        {/* Time display */}
        <div style={{
          fontFamily: "var(--font-mono)", fontSize: 68, fontWeight: 300,
          textAlign: "center", letterSpacing: 4, lineHeight: 1,
          color: invalid ? "var(--color-warm)" : digits.length === 0 ? "var(--color-text-3)" : "var(--color-text)",
          transition: "color 0.15s",
        }}>
          {displayStr}
        </div>

        {/* Numpad */}
        <div style={{ display: "grid", gridTemplateColumns: "repeat(3, 1fr)", gap: 9 }}>
          {keypad.flat().map((k, idx) => {
            if (k === null) return <div key={idx} />
            return (
              <button key={idx} onClick={() => k === "⌫" ? del() : push(k as number)} style={numBtnStyle(k)}>
                {k}
              </button>
            )
          })}
        </div>

        {/* Player */}
        <PlayerPicker value={playerId} onChange={setPlayerId} />

        {/* Start */}
        <button
          onClick={() => { if (valid) { onStart(total, playerId); onClose() } }}
          disabled={!valid}
          style={{
            padding: "14px", borderRadius: 12,
            background: valid ? "var(--color-accent)" : "var(--color-surface-3)",
            border: `1px solid ${valid ? "var(--color-accent)" : "var(--color-border)"}`,
            color: valid ? "var(--color-background)" : "var(--color-text-3)",
            fontSize: 15, fontWeight: 700,
            fontFamily: "var(--font-ui)", letterSpacing: 1,
            transition: "all 0.15s",
          }}
        >
          {valid ? `▶  Start  ${displayStr}` : "Enter a duration above"}
        </button>

        {/* Scheduled timers from backend */}
        {timers.length > 0 && (
          <>
            <div style={{ height: 1, background: "var(--color-border)" }} />
            <div>
              <div style={{ fontSize: 10, letterSpacing: 2.5, textTransform: "uppercase", color: "var(--color-text-3)", fontFamily: "var(--font-mono)", marginBottom: 10 }}>Scheduled</div>
              <div style={{ display: "flex", flexDirection: "column", gap: 8 }}>
                {timers.map(t => {
                  const secsLeft = Math.max(0, Math.round(t.trigger_at - Date.now() / 1000))
                  const player = PLAYERS.find(p => p.player_id === t.player_id)
                  return (
                    <div key={t.id} style={{
                      display: "flex", alignItems: "center", justifyContent: "space-between",
                      background: "var(--color-surface-2)", border: "1px solid var(--color-border)",
                      borderRadius: 10, padding: "10px 14px",
                    }}>
                      <div>
                        <div style={{ fontFamily: "var(--font-mono)", fontSize: 20, fontWeight: 300 }}>{fmtDur(secsLeft)}</div>
                        <div style={{ fontSize: 11, color: "var(--color-text-3)", marginTop: 2 }}>{player?.name ?? t.player_id}</div>
                      </div>
                      <button onClick={() => onCancelTimer(t.id)} style={{
                        width: 32, height: 32, borderRadius: 7,
                        background: C.warm.bg, border: `1px solid ${C.warm.border}`,
                        color: C.warm.text, fontSize: 14,
                        display: "flex", alignItems: "center", justifyContent: "center",
                      }}>✕</button>
                    </div>
                  )
                })}
              </div>
            </div>
          </>
        )}
      </div>
    </div>
  )
}

// ─── TimerCard ────────────────────────────────────────────────────────────────

function TimerCard({ timers, onDispatch, onCancelTimer, onTimerDone }: {
  timers: ScheduleEntry[]
  onDispatch: (duration_seconds: number, player_id: string) => void
  onCancelTimer: (id: string) => void
  onTimerDone: (player_id: string) => void
}) {
  const [modalOpen, setModalOpen] = useState(false)
  const [duration, setDuration]   = useState(0)
  const [remaining, setRemaining] = useState(0)
  const [running, setRunning]     = useState(false)
  const [playerId, setPlayerId]   = useState(PLAYERS[0].player_id)

  const idle = duration === 0
  const done = duration > 0 && remaining === 0

  useEffect(() => {
    if (!running) return
    const t = setInterval(() => {
      setRemaining(r => {
        if (r <= 1) { setRunning(false); return 0 }
        return r - 1
      })
    }, 1000)
    return () => clearInterval(t)
  }, [running])

  useEffect(() => {
    if (done) onTimerDone(playerId)
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [done])

  function start(secs: number, pid: string) {
    setDuration(secs); setRemaining(secs); setPlayerId(pid)
    setRunning(true)
    onDispatch(secs, pid)
  }

  function stop() { setRunning(false); setRemaining(0); setDuration(0) }

  const progress     = duration > 0 ? remaining / duration : 1
  const timeStr      = `${pad(Math.floor(remaining / 60))}:${pad(remaining % 60)}`
  const R            = 42
  const circumference = 2 * Math.PI * R

  const state = idle ? "idle" : done ? "done" : running ? "running" : "paused"

  const cardBg     = state === "running" ? C.accent.bg : state === "done" ? C.warm.bg : "var(--color-surface-2)"
  const cardBorder = state === "running" ? C.accent.border : state === "done" ? C.warm.border : "var(--color-border)"
  const cardGlow   = state === "running" ? `0 0 26px ${C.accent.bg}` : state === "done" ? `0 0 26px ${C.warm.bg}` : "none"
  const ringColor  = state === "running" ? "var(--color-accent)" : state === "done" ? "var(--color-warm)" : "var(--color-border)"

  return (
    <>
      <div
        onClick={() => (idle || done) ? setModalOpen(true) : undefined}
        style={{
          background: cardBg, border: `1px solid ${cardBorder}`,
          borderRadius: 16, padding: "22px 24px",
          display: "flex", flexDirection: "column", justifyContent: "space-between", alignItems: "flex-start",
          height: "100%", width: "100%",
          transition: "all 0.2s ease",
          boxShadow: cardGlow,
          cursor: idle || done ? "pointer" : "default",
          textAlign: "left",
        }}
      >
        {(idle || done) ? (
          <>
            <span style={{ fontSize: 42 }}>⏱</span>
            <div>
              <div style={{ fontSize: 18, fontWeight: 600, color: done ? C.warm.text : "var(--color-text)" }}>Timer</div>
              <div style={{ fontSize: 11, fontFamily: "var(--font-mono)", marginTop: 3,
                color: done ? C.warm.text : "var(--color-text-3)",
                textTransform: "uppercase", letterSpacing: 1.5 }}>
                {done ? "DONE · TAP TO SET" : "TAP TO SET"}
              </div>
            </div>
          </>
        ) : (
          <>
            {/* Header row: label + pause/stop */}
            <div style={{ display: "flex", alignItems: "center", justifyContent: "space-between", width: "100%" }}>
              <div>
                <div style={{ fontSize: 14, fontWeight: 600, color: "var(--color-text)" }}>Timer</div>
                <div style={{ fontSize: 10, fontFamily: "var(--font-mono)", marginTop: 1,
                  color: running ? C.accent.text : "var(--color-text-3)",
                  textTransform: "uppercase", letterSpacing: 1.2 }}>
                  {running ? "● running" : "○ paused"}
                </div>
              </div>
              <div style={{ display: "flex", gap: 6 }}>
                <button onClick={e => { e.stopPropagation(); setRunning(r => !r) }} style={{
                  width: 32, height: 32, borderRadius: 8,
                  background: "var(--color-surface-3)", border: "1px solid var(--color-border-bright)",
                  color: "var(--color-text-2)", fontSize: 14,
                  display: "flex", alignItems: "center", justifyContent: "center",
                }}>{running ? "⏸" : "▶"}</button>
                <button onClick={e => { e.stopPropagation(); stop() }} style={{
                  width: 32, height: 32, borderRadius: 8,
                  background: C.warm.bg, border: `1px solid ${C.warm.border}`,
                  color: C.warm.text, fontSize: 13,
                  display: "flex", alignItems: "center", justifyContent: "center",
                }}>✕</button>
              </div>
            </div>

            {/* Arc ring */}
            <div style={{ flex: 1, display: "flex", alignItems: "center", justifyContent: "center", width: "100%", position: "relative", minHeight: 96 }}>
              <svg width="96" height="96" viewBox="0 0 96 96" style={{ position: "absolute" }}>
                <circle cx="48" cy="48" r={R} fill="none" stroke="var(--color-surface-3)" strokeWidth="3" />
                <circle cx="48" cy="48" r={R} fill="none"
                  stroke={ringColor} strokeWidth="3"
                  strokeDasharray={circumference}
                  strokeDashoffset={circumference * (1 - progress)}
                  strokeLinecap="round"
                  transform="rotate(-90 48 48)"
                  style={{ transition: "stroke-dashoffset 0.85s linear, stroke 0.3s" }}
                />
              </svg>
              <div style={{
                fontFamily: "var(--font-mono)", fontSize: 26, fontWeight: 300, letterSpacing: 3, lineHeight: 1,
                color: running ? "var(--color-accent)" : "var(--color-text)",
                position: "relative",
              }}>{timeStr}</div>
            </div>

            {/* Player name */}
            <div style={{ fontSize: 10, color: "var(--color-text-3)", fontFamily: "var(--font-mono)", letterSpacing: 1 }}>
              {PLAYERS.find(p => p.player_id === playerId)?.name ?? playerId}
            </div>
          </>
        )}
      </div>

      {modalOpen && (
        <TimerKeypadModal
          timers={timers}
          onClose={() => setModalOpen(false)}
          onStart={start}
          onCancelTimer={onCancelTimer}
        />
      )}
    </>
  )
}

function HomeTab({ alarms, timers, onOpenAlarm, onDispatchTimer, onCancelTimer, onTimerDone }: {
  alarms: ScheduleEntry[]
  timers: ScheduleEntry[]
  onOpenAlarm: () => void
  onDispatchTimer: (duration_seconds: number, player_id: string) => void
  onCancelTimer: (id: string) => void
  onTimerDone: (player_id: string) => void
}) {
  const [toggles, setToggles] = useState<Record<string, boolean>>({})
  const [pulsing, setPulsing] = useState<string | null>(null)
  const [vacuumState]         = useState("docked")

  function tog(id: string)   { setToggles(p => ({ ...p, [id]: !p[id] })) }
  function pulse(id: string) { setPulsing(id); setTimeout(() => setPulsing(null), 1800) }

  return (
    <div style={{ display: "flex", width: "100%", height: "100%", alignItems: "stretch", padding: "24px 40px" }}>
      <div style={{
        display: "grid", gridTemplateColumns: "repeat(3, 1fr)", gridTemplateRows: "repeat(2, 1fr)",
        gap: 16, width: "100%", height: "100%",
      }}>
        <RoboCard vacuumState={vacuumState} />
        <SimpleCard icon="😎" label="The Dude"   color="accent"  toggle isOn={toggles["dude"]} isSent={false} onPress={() => tog("dude")} />
        <SimpleCard icon="📳" label="Find Phone" color="warm"   toggle={false} isOn={false} isSent={pulsing === "find_phone"} onPress={() => pulse("find_phone")} />
        <SimpleCard icon="🪞" label="Mirror"     color="neutral" toggle={false} isOn={false} isSent={pulsing === "mirror"}    onPress={() => pulse("mirror")} />
        <AlarmCard alarms={alarms} onOpen={onOpenAlarm} />
        <TimerCard timers={timers} onDispatch={onDispatchTimer} onCancelTimer={onCancelTimer} onTimerDone={onTimerDone} />
      </div>
    </div>
  )
}

// ─── Tab 3: Music ─────────────────────────────────────────────────────────────

const TransportBtn = ({ icon, label, big, active, activeColor, onClick }: {
  icon: string; label: string; big?: boolean
  active?: boolean; activeColor?: string; onClick: () => void
}) => (
  <button onClick={onClick} title={label} style={{
    width: big ? 60 : 46, height: big ? 60 : 46,
    borderRadius: "50%", flexShrink: 0,
    background: big ? "var(--color-accent)" : !!active ? "transparent" : "var(--color-surface-3)",
    border: big ? "none" : `1px solid ${!!active && activeColor ? activeColor : "var(--color-border)"}`,
    color: big ? "var(--color-background)" : !!active && activeColor ? activeColor : "var(--color-text-2)",
    fontSize: big ? 20 : 17,
    display: "flex", alignItems: "center", justifyContent: "center",
    transition: "all 0.15s ease",
  }}>{icon}</button>
)

function MusicTab() {
  const [track, setTrack]       = useState<TrackInfo>(MOCK_TRACK)
  const [queue]                  = useState<QueueItem[]>(MOCK_QUEUE)
  const [activeId, setActiveId]  = useState<string | null>(null)
  const [rightView, setRightView]= useState<"queue" | "playlists">("queue")
  const [playerId, setPlayerId]  = useState(PLAYERS[0].player_id)

  useEffect(() => {
    if (!track.playing) return
    const t = setInterval(() => {
      setTrack(p => {
        const next = p.position + 1
        return next >= p.duration ? { ...p, position: 0, playing: false } : { ...p, position: next }
      })
    }, 1000)
    return () => clearInterval(t)
  }, [track.playing])

  const progress = track.duration > 0 ? (track.position / track.duration) * 100 : 0

  function seek(e: React.MouseEvent<HTMLDivElement>) {
    const rect = e.currentTarget.getBoundingClientRect()
    setTrack(p => ({ ...p, position: Math.round(((e.clientX - rect.left) / rect.width) * p.duration) }))
  }

  function playFromQueue(item: QueueItem) {
    setActiveId(item.id)
    setTrack(p => ({ ...p, title: item.title, artist: item.artist, position: 0, playing: true }))
  }

  function playPlaylist(uri: string) {
    // TODO: mqtt publish dashboard/cmd/play_playlist with uri and playerId via dashboard/cmd/select_player
    console.log("play_playlist", uri, "player", playerId)
  }

  function selectPlayer(id: string) {
    setPlayerId(id)
    // TODO: mqtt publish dashboard/cmd/select_player with id
  }

  return (
    <div style={{ display: "flex", width: "100%", height: "100%", gap: 0 }}>
      {/* Left: Player */}
      <div style={{
        width: 580, flexShrink: 0, borderRight: "1px solid var(--color-border)",
        display: "flex", flexDirection: "column", justifyContent: "flex-start",
        padding: "16px 40px", gap: 0, overflow: "hidden",
      }}>
        {/* Album art — full column interior width, square */}
        <div style={{
          width: 500, height: 500, borderRadius: 16, flexShrink: 0,
          background: "var(--color-surface-3)", border: "1px solid var(--color-border)",
          display: "flex", alignItems: "center", justifyContent: "center",
          fontSize: 80,
        }}>🎵</div>

        {/* Title + artist */}
        <div style={{ marginTop: 10, minWidth: 0 }}>
          <div style={{ fontSize: 18, fontWeight: 600, overflow: "hidden", textOverflow: "ellipsis", whiteSpace: "nowrap" }}>{track.title}</div>
          <div style={{ fontSize: 13, color: "var(--color-text-2)", marginTop: 2, overflow: "hidden", textOverflow: "ellipsis", whiteSpace: "nowrap" }}>{track.artist}</div>
        </div>

        {/* Seekbar */}
        <div style={{ marginTop: 12 }}>
          <div onClick={seek} style={{ height: 4, background: "var(--color-surface-3)", borderRadius: 2, cursor: "pointer", position: "relative", marginBottom: 6 }}>
            <div style={{ position: "absolute", left: 0, top: 0, bottom: 0, width: `${progress}%`, background: "var(--color-accent)", borderRadius: 2, transition: "width 0.5s linear" }} />
          </div>
          <div style={{ display: "flex", justifyContent: "space-between", fontFamily: "var(--font-mono)", fontSize: 11, color: "var(--color-text-3)" }}>
            <span>{fmtDur(track.position)}</span>
            <span>{fmtDur(track.duration)}</span>
          </div>
        </div>

        {/* Transport */}
        <div style={{ display: "flex", alignItems: "center", justifyContent: "center", gap: 10, marginTop: 10 }}>
          <TransportBtn icon={track.favorited ? "♥" : "♡"} label="Fav" active={track.favorited} activeColor="var(--color-warm)" onClick={() => setTrack(p => ({ ...p, favorited: !p.favorited }))} />
          <TransportBtn icon="⏮" label="Prev" onClick={() => {}} />
          <TransportBtn icon={track.playing ? "⏸" : "▶"} label="Play" big onClick={() => setTrack(p => ({ ...p, playing: !p.playing }))} />
          <TransportBtn icon="⏭" label="Next" onClick={() => {}} />
          <TransportBtn icon="⇄" label="Shuffle" active={track.shuffle} activeColor="var(--color-accent)" onClick={() => setTrack(p => ({ ...p, shuffle: !p.shuffle }))} />
          <TransportBtn icon="📻" label="Radio" onClick={() => {}} />
        </div>

        {/* Player picker + Volume — same row */}
        <div style={{ display: "flex", alignItems: "center", gap: 16, marginTop: 12 }}>
          <PlayerPicker value={playerId} onChange={selectPlayer} />
          <div style={{ flex: 1, display: "flex", alignItems: "center", gap: 8 }}>
            <span style={{ fontSize: 10, color: "var(--color-text-3)", fontFamily: "var(--font-mono)", flexShrink: 0 }}>VOL</span>
            <div style={{ flex: 1, height: 4, background: "var(--color-surface-3)", borderRadius: 2, cursor: "pointer", position: "relative" }}
              onClick={e => {
                const rect = e.currentTarget.getBoundingClientRect()
                setTrack(p => ({ ...p, volume: Math.round(((e.clientX - rect.left) / rect.width) * 100) }))
              }}>
              <div style={{ position: "absolute", left: 0, top: 0, bottom: 0, width: `${track.volume}%`, background: "var(--color-accent)", borderRadius: 2, opacity: 0.7 }} />
            </div>
            <span style={{ fontSize: 11, color: "var(--color-text-2)", fontFamily: "var(--font-mono)", width: 24, textAlign: "right", flexShrink: 0 }}>{track.volume}</span>
          </div>
        </div>
      </div>

      {/* Right: Queue / Playlists */}
      <div style={{ flex: 1, display: "flex", flexDirection: "column", padding: "24px 32px 0", minWidth: 0, minHeight: 0, overflow: "hidden" }}>
        {/* Toggle */}
        <div style={{ display: "flex", gap: 3, marginBottom: 18, background: "var(--color-surface-2)", border: "1px solid var(--color-border)", borderRadius: 10, padding: 3, alignSelf: "flex-start" }}>
          {(["queue", "playlists"] as const).map(v => (
            <button key={v} onClick={() => setRightView(v)} style={{
              padding: "6px 18px", borderRadius: 7,
              background: rightView === v ? "var(--color-accent)" : "transparent",
              border: "none",
              color: rightView === v ? "var(--color-background)" : "var(--color-text-2)",
              fontSize: 12, fontWeight: rightView === v ? 600 : 400, fontFamily: "var(--font-ui)",
              transition: "all 0.15s",
            }}>{v === "queue" ? "Up Next" : "Playlists"}</button>
          ))}
        </div>

        <div style={{ flex: 1, display: "flex", flexDirection: "column", gap: 4, overflowY: "auto", minHeight: 0, paddingBottom: 20 }}>
          {rightView === "queue" ? (
            queue.map((item, i) => {
              const isCurrent = activeId === item.id || (activeId === null && i === 0)
              return (
                <button key={item.id} onClick={() => playFromQueue(item)} style={{
                  display: "flex", alignItems: "center", gap: 14, padding: "12px 14px",
                  borderRadius: 10,
                  background: isCurrent ? "rgba(0,200,240,0.1)" : "transparent",
                  border: `1px solid ${isCurrent ? "var(--color-accent)" : "transparent"}`,
                  textAlign: "left", transition: "background 0.12s",
                }}>
                  <span style={{ fontFamily: "var(--font-mono)", fontSize: 11, color: "var(--color-text-3)", width: 18, textAlign: "right" }}>{i + 1}</span>
                  <div style={{ flex: 1, minWidth: 0 }}>
                    <div style={{ fontSize: 14, fontWeight: 500, color: isCurrent ? "var(--color-accent)" : "var(--color-text)", overflow: "hidden", textOverflow: "ellipsis", whiteSpace: "nowrap" }}>{item.title}</div>
                    <div style={{ fontSize: 12, color: "var(--color-text-3)", marginTop: 1 }}>{item.artist}</div>
                  </div>
                  <span style={{ fontFamily: "var(--font-mono)", fontSize: 11, color: "var(--color-text-3)", flexShrink: 0 }}>{fmtDur(item.duration)}</span>
                </button>
              )
            })
          ) : (
            PLAYLISTS.map(pl => (
              <button key={pl.uri} onClick={() => playPlaylist(pl.uri)} style={{
                display: "flex", alignItems: "center", gap: 14, padding: "14px 16px",
                borderRadius: 10,
                background: "var(--color-surface-2)", border: "1px solid var(--color-border)",
                textAlign: "left",
                transition: "all 0.12s",
              }}>
                <div style={{
                  width: 38, height: 38, borderRadius: 8, flexShrink: 0,
                  background: "var(--color-surface-3)", border: "1px solid var(--color-border)",
                  display: "flex", alignItems: "center", justifyContent: "center", fontSize: 17,
                }}>🎵</div>
                <div style={{ flex: 1, minWidth: 0 }}>
                  <div style={{ fontSize: 14, fontWeight: 500, color: "var(--color-text)", overflow: "hidden", textOverflow: "ellipsis", whiteSpace: "nowrap" }}>{pl.name}</div>
                  <div style={{ fontSize: 10, color: "var(--color-text-3)", fontFamily: "var(--font-mono)", marginTop: 2, letterSpacing: 0.5 }}>{pl.uri}</div>
                </div>
                <span style={{ color: "var(--color-accent)", fontSize: 14, flexShrink: 0 }}>▶</span>
              </button>
            ))
          )}
        </div>
      </div>
    </div>
  )
}

// ─── Ringing Popup ────────────────────────────────────────────────────────────

function RingingPopup({ ringing, onStop }: { ringing: RingingEntry[]; onStop: (id: string) => void }) {
  if (ringing.length === 0) return null
  const first   = ringing[0]
  const isAlarm = first.kind === "alarm"
  const player  = PLAYERS.find(p => p.player_id === first.player_id)
  const col     = isAlarm ? C.warm : C.accent

  return (
    <div style={{
      position: "absolute", inset: 0,
      background: "rgba(0,0,0,0.84)",
      display: "flex", alignItems: "center", justifyContent: "center",
      zIndex: 200,
    }}>
      <div style={{
        background: col.bg,
        border: `2px solid ${col.border}`,
        borderRadius: 26, padding: "52px 72px",
        display: "flex", flexDirection: "column", alignItems: "center", gap: 20,
        boxShadow: `0 0 100px ${col.bg}, 0 0 40px ${col.bg}`,
        minWidth: 360,
      }}>
        <span style={{ fontSize: 68 }}>{isAlarm ? "⏰" : "⏱"}</span>
        <div style={{ fontFamily: "var(--font-mono)", fontSize: 24, fontWeight: 700, color: col.text, letterSpacing: 4, textTransform: "uppercase" }}>
          {isAlarm ? "Alarm" : "Timer Done"}
        </div>
        {player && (
          <div style={{ fontSize: 12, color: "var(--color-text-2)", fontFamily: "var(--font-mono)", letterSpacing: 2, textTransform: "uppercase" }}>
            {player.name}
          </div>
        )}
        <button
          onClick={() => ringing.forEach(r => onStop(r.id))}
          style={{
            marginTop: 10, padding: "20px 72px", borderRadius: 16,
            background: isAlarm ? "var(--color-warm)" : "var(--color-accent)",
            border: "none",
            color: "var(--color-background)", fontSize: 22, fontWeight: 800,
            fontFamily: "var(--font-ui)", letterSpacing: 4,
          }}
        >STOP</button>
        {ringing.length > 1 && (
          <div style={{ fontSize: 12, color: "var(--color-text-3)", fontFamily: "var(--font-mono)" }}>
            +{ringing.length - 1} more ringing
          </div>
        )}
      </div>
    </div>
  )
}

// ─── Root ─────────────────────────────────────────────────────────────────────

const APP_W = 1280
const APP_H = 800

function useScale() {
  const [scale, setScale] = useState(() =>
    Math.min(window.innerWidth / APP_W, window.innerHeight / APP_H)
  )
  useEffect(() => {
    const update = () => setScale(Math.min(window.innerWidth / APP_W, window.innerHeight / APP_H))
    window.addEventListener("resize", update)
    return () => window.removeEventListener("resize", update)
  }, [])
  return scale
}

export default function App() {
  const [tab, setTab]                       = useState<Tab>("clock")
  const scale                               = useScale()
  const [ringing, setRinging]               = useState<RingingEntry[]>([])
  const [alarms,  setAlarms]                = useState<ScheduleEntry[]>([])
  const [timers,  setTimers]                = useState<ScheduleEntry[]>([])
  const [alarmModalOpen, setAlarmModalOpen] = useState(false)

  function handleSetAlarm(hh: number, mm: number, player_id: string) {
    const now = new Date()
    const trigger = new Date(now)
    trigger.setHours(hh, mm, 0, 0)
    if (trigger <= now) trigger.setDate(trigger.getDate() + 1)
    setAlarms(a => [...a, { id: uid(), kind: "alarm", trigger_at: trigger.getTime() / 1000, player_id }])
    // TODO: mqtt publish dashboard/cmd/set_alarm {time: `${pad(hh)}:${pad(mm)}`, player_id}
  }

  function handleCancelAlarm(id: string) {
    setAlarms(a => a.filter(e => e.id !== id))
    // TODO: mqtt publish dashboard/cmd/cancel_alarm <id>
  }

  function handleDispatchTimer(duration_seconds: number, player_id: string) {
    setTimers(t => [...t, { id: uid(), kind: "timer", trigger_at: Date.now() / 1000 + duration_seconds, player_id }])
    // TODO: mqtt publish dashboard/cmd/set_timer {duration_seconds, player_id}
  }

  function handleCancelTimer(id: string) {
    setTimers(t => t.filter(e => e.id !== id))
    // TODO: mqtt publish dashboard/cmd/cancel_timer <id>
  }

  function handleTimerDone(player_id: string) {
    setRinging(r => [...r, { id: uid(), kind: "timer", player_id }])
    // In production this comes from dashboard/state/ringing via MQTT
  }

  function handleStopRinging(id: string) {
    setRinging(r => r.filter(e => e.id !== id))
    // TODO: mqtt publish dashboard/cmd/stop_timer or stop_alarm <id>
  }

  return (
    <div style={{
      width: "100vw", height: "100vh",
      background: "var(--color-background)",
      display: "flex", alignItems: "center", justifyContent: "center",
      overflow: "hidden",
    }}>
      <div style={{
        width: APP_W, height: APP_H, flexShrink: 0,
        background: "var(--color-background)", color: "var(--color-text)",
        display: "flex", flexDirection: "column", overflow: "hidden",
        transform: `scale(${scale})`, transformOrigin: "center center",
        position: "relative",
      }}>
        {/* Tab bar */}
        <div style={{
          height: 68, flexShrink: 0,
          display: "flex", alignItems: "center", justifyContent: "center",
          borderBottom: "1px solid var(--color-border)", background: "var(--color-surface)",
        }}>
          <TabBar active={tab} onChange={setTab} />
        </div>

        {/* Content */}
        <div style={{ flex: 1, overflow: "hidden", display: "flex", width: "100%" }}>
          {tab === "clock" && <ClockWeatherTab />}
          {tab === "home"  && (
            <HomeTab
              alarms={alarms}
              timers={timers}
              onOpenAlarm={() => setAlarmModalOpen(true)}
              onDispatchTimer={handleDispatchTimer}
              onCancelTimer={handleCancelTimer}
              onTimerDone={handleTimerDone}
            />
          )}
          {tab === "music" && <MusicTab />}
        </div>

        {/* Overlays — rendered inside canvas so they scale with it */}
        {alarmModalOpen && (
          <AlarmModal
            alarms={alarms}
            onClose={() => setAlarmModalOpen(false)}
            onSet={handleSetAlarm}
            onCancel={handleCancelAlarm}
          />
        )}
        <RingingPopup ringing={ringing} onStop={handleStopRinging} />
      </div>
    </div>
  )
}
