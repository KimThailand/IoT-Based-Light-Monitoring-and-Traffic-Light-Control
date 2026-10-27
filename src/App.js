import { useEffect, useState } from "react";
import mqtt from "mqtt";
import "./App.css";

const BROKER_URL = process.env.REACT_APP_MQTT_URL;
const LIGHT_TOPIC = process.env.REACT_APP_MQTT_LIGHT_TOPIC;
const TRAFFIC_STATUS_TOPIC = process.env.REACT_APP_MQTT_TRAFFIC_STATUS_TOPIC;
const TRAFFIC_SET_TOPIC = process.env.REACT_APP_MQTT_TRAFFIC_SET_TOPIC;

// ปรับช่วงตามสภาพแสงจริงที่ใช้งาน เช่น ในบ้าน ~50-500 lux, กลางแจ้งอาจเกิน 10,000 lux
const SCALE_MIN = 0;
const SCALE_MAX = 1000;

const COLORS = ["RED", "YELLOW", "GREEN"];

export default function App() {
  const [status, setStatus] = useState("connecting");
  const [mqttClient, setMqttClient] = useState(null);

  const [lux, setLux] = useState(null);
  const [lightLastUpdate, setLightLastUpdate] = useState(null);

  const [trafficColor, setTrafficColor] = useState(null);
  const [trafficLastUpdate, setTrafficLastUpdate] = useState(null);

  const [now, setNow] = useState(Date.now());

  useEffect(() => {
    const client = mqtt.connect(BROKER_URL, {
      username: process.env.REACT_APP_MQTT_USER,
      password: process.env.REACT_APP_MQTT_PASS,
      clientId: `web-${Math.random().toString(16).slice(2, 10)}`,
      reconnectPeriod: 3000,
    });

    client.on("connect", () => {
      console.log("MQTT Connected");
      setStatus("connected");

      client.subscribe([LIGHT_TOPIC, TRAFFIC_STATUS_TOPIC], (err) => {
        if (err) {
          console.error("Subscribe failed:", err);
        } else {
          console.log("Subscribed to topics");
        }
      });
    });

    client.on("message", (topic, payload) => {
      const text = payload.toString();
      console.log("Received:", topic, text);

      if (topic === LIGHT_TOPIC) {
        const value = parseFloat(text);
        if (!Number.isNaN(value)) {
          setLux(value);
          setLightLastUpdate(Date.now());
        }
      }

      if (topic === TRAFFIC_STATUS_TOPIC) {
        const color = text.trim().toUpperCase();
        if (COLORS.includes(color)) {
          setTrafficColor(color);
          setTrafficLastUpdate(Date.now());
        }
      }
    });

    client.on("reconnect", () => setStatus("reconnecting"));
    client.on("offline", () => setStatus("offline"));
    client.on("error", (err) => {
      console.error("MQTT Error:", err);
      setStatus("error");
    });

    setMqttClient(client);

    return () => {
      client.end();
    };
  }, []);

  useEffect(() => {
    const timer = setInterval(() => {
      setNow(Date.now());
    }, 1000);

    return () => clearInterval(timer);
  }, []);

  const sendTrafficCommand = (color) => {
    if (mqttClient && mqttClient.connected) {
      mqttClient.publish(TRAFFIC_SET_TOPIC, color, { qos: 0, retain: false });
    }
  };

  const formatTime = (ts) =>
    ts ? new Date(ts).toLocaleTimeString("th-TH", { hour12: false }) : "--";

  const secondsAgo = (ts) => (ts ? Math.floor((now - ts) / 1000) : null);

  const lightSecondsAgo = secondsAgo(lightLastUpdate);
  const trafficSecondsAgo = secondsAgo(trafficLastUpdate);

  const fill =
    lux !== null
      ? Math.min(1, Math.max(0, (lux - SCALE_MIN) / (SCALE_MAX - SCALE_MIN)))
      : 0;

  return (
    <main className="stage">
      {/* ---------- Light Sensor Gauge ---------- */}
      <section className="panel light-panel">
        <div className="thermo-row">
          <div className={`thermo ${lux === null ? "empty" : ""}`}>
            <div className="scale">
              <span>{SCALE_MAX}</span>
              <span>{Math.round((SCALE_MAX + SCALE_MIN) / 2)}</span>
              <span>{SCALE_MIN}</span>
            </div>

            <div className="gauge">
              <div className="tube">
                <div
                  className="mercury"
                  style={{ height: `${fill * 100}%` }}
                />
              </div>
              <div className="bulb" />
            </div>
          </div>

          <div className="readout">
            <h1>Light Value</h1>

            <p className="temp">
              {lux !== null ? lux.toFixed(1) : "--"}
              <span>lux</span>
            </p>

            <p className="meta">Last update time: {formatTime(lightLastUpdate)}</p>
            <p className="meta">
              {lightSecondsAgo !== null
                ? `Updated ${lightSecondsAgo} s ago`
                : "Waiting for data from ESP8266…"}
            </p>
          </div>
        </div>
      </section>

      {/* ---------- Traffic Light Control ---------- */}
      <section className="panel traffic-panel">
        <h1>Traffic Light Control</h1>

        <div className="traffic-lamp">
          {COLORS.map((c) => (
            <span
              key={c}
              className={`lamp ${c.toLowerCase()} ${
                trafficColor === c ? "active" : ""
              }`}
            />
          ))}
        </div>

        <p className="current-color" data-color={trafficColor || ""}>
          Current: {trafficColor || "--"}
        </p>

        <div className="traffic-buttons">
          {COLORS.map((c) => (
            <button
              key={c}
              className={`btn ${c.toLowerCase()} ${
                trafficColor === c ? "active" : ""
              }`}
              onClick={() => sendTrafficCommand(c)}
            >
              {c}
            </button>
          ))}
        </div>

        <p className="meta">Last update time: {formatTime(trafficLastUpdate)}</p>
        <p className="meta">
          {trafficSecondsAgo !== null
            ? `Updated ${trafficSecondsAgo} s ago`
            : "Waiting for data from ESP8266…"}
        </p>
      </section>

      <p className="status" data-state={status}>
        {status === "connected" && "Connected to broker"}
        {status === "connecting" && "Connecting to broker…"}
        {status === "reconnecting" && "Connection lost. Retrying…"}
        {status === "offline" && "Offline"}
        {status === "error" && "Can't connect to broker"}
      </p>
    </main>
  );
}