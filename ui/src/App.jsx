import { useEffect, useState } from "react";
import "./App.css";

export default function App() {
  const [client, setClient] = useState("CLIENT1");
  const [side, setSide] = useState("BUY");
  const [symbol, setSymbol] = useState("EAG");
  const [qty, setQty] = useState(100);
  const [type, setType] = useState("MARKET");
  const [price, setPrice] = useState(1.0);

  const [orders, setOrders] = useState([]);

  const GATEWAY = `${location.hostname}:8080`;

  useEffect(() => {
    const ws = new WebSocket(`ws://${GATEWAY}/ws`);
    ws.onmessage = ({ data }) => {
      const { id, status } = JSON.parse(data);
      setOrders(prev =>
        prev.map(o => (o.id === id ? { ...o, status } : o))
      );
    };
    return () => ws.close();
  }, []);

  const statusText = s =>
    s === "0" ? "NEW" :
    s === "1" ? "PARTIAL" :
    s === "2" || s === "F" ? "FILLED" :
    s === "4" ? "CANCELED" :
    s === "8" ? "REJECTED" : s;

  const submit = async () => {
    try {
      const body = {
        client,
        side,
        symbol,
        qty: Number(qty),
        ...(type === "LIMIT" && { price: Number(price) })
      };

      const res = await fetch(`http://${GATEWAY}/api/order`, {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify(body)
      });
      if (!res.ok) throw new Error(await res.text());

      const { clientOrderId: id } = await res.json();
      setOrders(p => [
        ...p,
        {
          key: crypto.randomUUID(),
          id,
          symbol,
          qty: Number(qty),
          status: "0" 
        }
      ]);
    } catch (e) {
      alert("Order failed: " + e.message);
    }
  };

  




  return (
    <div className="wrap">
      <h2>Wef Trading</h2>

      <div className="form">
        <label>
          Client
          <select value={client} onChange={e=>setClient(e.target.value)}>
            <option>CLIENT1</option>
            <option>CLIENT2</option>
          </select>
        </label>

        <label>
          Side
          <select value={side} onChange={e=>setSide(e.target.value)}>
            <option value="BUY">BUY</option>
            <option value="SELL">SELL</option>
          </select>
        </label>

        <label>
          Symbol
          <input value={symbol}
                 onChange={e=>setSymbol(e.target.value.toUpperCase())} />
        </label>

        <label>
          Qty
          <input type="number" min="1" value={qty}
                 onChange={e=>setQty(e.target.value)} />
        </label>

        <label>
          Type
          <select value={type} onChange={e=>setType(e.target.value)}>
            <option value="MARKET">Market</option>
            <option value="LIMIT">Limit</option>
          </select>
        </label>

        {type === "LIMIT" && (
          <label>
            Price
            <input type="number" min="0" step="0.01"
                   value={price}
                   onChange={e=>setPrice(e.target.value)} />
          </label>
        )}

        <button onClick={submit}>Submit</button>
      </div>

      <table>
        <thead>
          <tr><th>ID</th><th>Sym</th><th>Qty</th><th>Status</th></tr>
        </thead>
        <tbody>
          {orders.map(o => (
            <tr key={o.key}
                className={["2","F"].includes(o.status) ? "filled" : ""}>
              <td>{o.id}</td>
              <td>{o.symbol}</td>
              <td>{o.qty}</td>
              <td>{statusText(o.status)}</td>
            </tr>
          ))}
        </tbody>
      </table>
    </div>
  );
}
