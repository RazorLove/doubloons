#ifndef WALLETMODEL_H
#define WALLETMODEL_H

#include <QObject>

#include "allocators.h" /* for SecureString */

class OptionsModel;
class AddressTableModel;
class TransactionTableModel;
class CWallet;

QT_BEGIN_NAMESPACE
class QTimer;
QT_END_NAMESPACE

class SendCoinsRecipient
{
public:
    QString address;
    QString label;
    qint64 amount;
};

/** Interface to Bitcoin chest from Qt view code. */
class WalletModel : public QObject
{
    Q_OBJECT
public:
    explicit WalletModel(CWallet *chest, OptionsModel *optionsModel, QObject *parent = 0);
    ~WalletModel();

    enum StatusCode // Returned by sendCoins
    {
        OK,
        InvalidAmount,
        InvalidAddress,
        AmountExceedsBalance,
        AmountWithFeeExceedsBalance,
        DuplicateAddress,
        TransactionCreationFailed, // Error returned when chest is still locked
        TransactionCommitFailed,
        Aborted
    };

    enum EncryptionStatus
    {
        Unencrypted,  // !chest->IsCrypted()
        Locked,       // chest->IsCrypted() && chest->IsLocked()
        Unlocked      // chest->IsCrypted() && !chest->IsLocked()
    };

    OptionsModel *getOptionsModel();
    AddressTableModel *getAddressTableModel();
    TransactionTableModel *getTransactionTableModel();

    qint64 getBalance() const;
    qint64 getUnconfirmedBalance() const;
    qint64 getImmatureBalance() const;
    int getNumTransactions() const;
    EncryptionStatus getEncryptionStatus() const;

    // Check address for validity
    bool validateAddress(const QString &address);

    // Return status record for SendCoins, contains error id + information
    struct SendCoinsReturn
    {
        SendCoinsReturn(StatusCode status,
                         qint64 fee=0,
                         QString hex=QString()):
            status(status), fee(fee), hex(hex) {}
        StatusCode status;
        qint64 fee; // is used in case status is "AmountWithFeeExceedsBalance"
        QString hex; // is filled with the transaction hash if status is "OK"
    };

    // Send coins to a list of recipients
    SendCoinsReturn sendCoins(const QList<SendCoinsRecipient> &recipients);

    // Chest encryption
    bool setWalletEncrypted(bool encrypted, const SecureString &passphrase);
    // Passphrase only needed when unlocking
    bool setWalletLocked(bool locked, const SecureString &passPhrase=SecureString());
    bool changePassphrase(const SecureString &oldPass, const SecureString &newPass);
    // Chest backup
    bool backupWallet(const QString &filename);

    // RAI object for unlocking chest, returned by requestUnlock()
    class UnlockContext
    {
    public:
        UnlockContext(WalletModel *chest, bool valid, bool relock);
        ~UnlockContext();

        bool isValid() const { return valid; }

        // Copy operator and constructor transfer the context
        UnlockContext(const UnlockContext& obj) { CopyFrom(obj); }
        UnlockContext& operator=(const UnlockContext& rhs) { CopyFrom(rhs); return *this; }
    private:
        WalletModel *chest;
        bool valid;
        mutable bool relock; // mutable, as it can be set to false by copying

        void CopyFrom(const UnlockContext& rhs);
    };

    UnlockContext requestUnlock();

private:
    CWallet *chest;

    // Chest has an options model for chest-specific options
    // (transaction fee, for example)
    OptionsModel *optionsModel;

    AddressTableModel *addressTableModel;
    TransactionTableModel *transactionTableModel;

    // Cache some values to be able to detect changes
    qint64 cachedBalance;
    qint64 cachedUnconfirmedBalance;
    qint64 cachedImmatureBalance;
    qint64 cachedNumTransactions;
    EncryptionStatus cachedEncryptionStatus;
    int cachedNumBlocks;

    QTimer *pollTimer;

    void subscribeToCoreSignals();
    void unsubscribeFromCoreSignals();
    void checkBalanceChanged();

signals:
    // Signal that balance in chest changed
    void balanceChanged(qint64 balance, qint64 unconfirmedBalance, qint64 immatureBalance);

    // Number of transactions in chest changed
    void numTransactionsChanged(int count);

    // Encryption status of chest changed
    void encryptionStatusChanged(int status);

    // Signal emitted when chest needs to be unlocked
    // It is valid behaviour for listeners to keep the chest locked after this signal;
    // this means that the unlocking failed or was cancelled.
    void requireUnlock();

    // Asynchronous error notification
    void error(const QString &title, const QString &message, bool modal);

public slots:
    /* Chest status might have changed */
    void updateStatus();
    /* New transaction, or transaction changed status */
    void updateTransaction(const QString &hash, int status);
    /* New, updated or removed address book entry */
    void updateAddressBook(const QString &address, const QString &label, bool isMine, int status);
    /* Current, immature or unconfirmed balance might have changed - emit 'balanceChanged' if so */
    void pollBalanceChanged();
};


#endif // WALLETMODEL_H
