// Copyright (c) 2011-2014 The Bitcoin Core developers
// Copyright (c) 2021 The Vivuscoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef VIVUSCOIN_QT_VIVUSCOINADDRESSVALIDATOR_H
#define VIVUSCOIN_QT_VIVUSCOINADDRESSVALIDATOR_H

#include <QValidator>

/** Base58 entry widget validator, checks for valid characters and
 * removes some whitespace.
 */
class VivuscoinAddressEntryValidator : public QValidator
{
    Q_OBJECT

public:
    explicit VivuscoinAddressEntryValidator(QObject *parent);

    State validate(QString &input, int &pos) const;
};

/** Vivuscoin address widget validator, checks for a valid vivuscoin address.
 */
class VivuscoinAddressCheckValidator : public QValidator
{
    Q_OBJECT

public:
    explicit VivuscoinAddressCheckValidator(QObject *parent);

    State validate(QString &input, int &pos) const;
};

#endif // VIVUSCOIN_QT_VIVUSCOINADDRESSVALIDATOR_H
