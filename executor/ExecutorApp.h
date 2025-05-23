#pragma once
#include <quickfix/Application.h>
#include <quickfix/MessageCracker.h>
#include <quickfix/fix42/NewOrderSingle.h>
#include <quickfix/fix42/ExecutionReport.h>

#include <functional>
#include <mutex>

class ExecutorApp : public FIX::Application,
                    public FIX::MessageCracker {
public:
  explicit ExecutorApp(
        std::function<void(const FIX42::ExecutionReport&)> execCb)
      : execCb_(std::move(execCb)), orderID_(0), execID_(0) {}

  void onCreate (const FIX::SessionID&) override {}
  void onLogon  (const FIX::SessionID& s) override { session_ = s; }
  void onLogout (const FIX::SessionID&) override {}
  void toAdmin  (FIX::Message&, const FIX::SessionID&) override {}
  void fromAdmin(const FIX::Message&, const FIX::SessionID&) 
  EXCEPT(FIX::FieldNotFound, FIX::IncorrectDataFormat, FIX::IncorrectTagValue, FIX::RejectLogon) override {}
  void toApp    (FIX::Message&, const FIX::SessionID&)
                      EXCEPT(FIX::DoNotSend) override {}

  void fromApp(const FIX::Message& m,const FIX::SessionID& s)
      EXCEPT(FIX::FieldNotFound, FIX::IncorrectDataFormat,
             FIX::IncorrectTagValue, FIX::UnsupportedMessageType) override {
    crack(m, s);
  }

  void onMessage(const FIX42::NewOrderSingle&,
                 const FIX::SessionID&) override;

private:
  std::string genOrderID() { return std::to_string(++orderID_); }
  std::string genExecID () { return std::to_string(++execID_ ); }

  FIX::SessionID  session_;
  std::mutex      m_;
  std::function<void(const FIX42::ExecutionReport&)> execCb_;
  int             orderID_, execID_;
};
