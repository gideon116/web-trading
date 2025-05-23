#pragma once
#include <quickfix/Application.h>
#include <quickfix/MessageCracker.h>
#include <quickfix/fix42/NewOrderSingle.h>
#include <quickfix/fix42/ExecutionReport.h>
#include <unordered_map>
#include <functional>
#include <mutex>

class TradeClientApp : public FIX::Application,
                       public FIX::MessageCracker {
public:
  using ExecCb = std::function<void(const FIX42::ExecutionReport&)>;

  explicit TradeClientApp(ExecCb cb)
      : execCb_(std::move(cb)) {}

  void sendNewOrder(const std::string& sender,
                    const std::string& clID,
                    const std::string& symbol,
                    int qty,
                    double px,
                    bool isMarket,
                    FIX::Side side);

  void onCreate (const FIX::SessionID&) override {}
  void onLogon  (const FIX::SessionID& s) override;
  void onLogout (const FIX::SessionID& s) override { sessionMap_.erase(s.getSenderCompID().getValue()); }
  void toAdmin  (FIX::Message&, const FIX::SessionID&) override {}
  void fromAdmin(const FIX::Message&, const FIX::SessionID&) 
      EXCEPT(FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::RejectLogon) override {}
  void toApp    (FIX::Message&, const FIX::SessionID&) EXCEPT(FIX::DoNotSend) override {}
  void fromApp  (const FIX::Message& m,const FIX::SessionID& s)
      EXCEPT(FIX::FieldNotFound, FIX::IncorrectDataFormat,
             FIX::IncorrectTagValue, FIX::UnsupportedMessageType) override {
    crack(m, s);
  }

  void onMessage(const FIX42::ExecutionReport& rpt,
                 const FIX::SessionID&) override { execCb_(rpt); }

private:
  std::mutex m_;
  std::unordered_map<std::string, FIX::SessionID> sessionMap_;
  ExecCb execCb_;
};