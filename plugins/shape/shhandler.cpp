// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com
/********************************************************************************
 * Author    :  Damir Bakiev                                                    *
 * Version   :  na                                                              *
 * Date      :  11 November 2021                                                *
 * Website   :  na                                                              *
 * Copyright :  Damir Bakiev 2016-2020                                          *
 * License:                                                                     *
 * Use, modification & distribution is subject to Boost Software License Ver 1. *
 * http://www.boost.org/LICENSE_1_0.txt                                         *
 ********************************************************************************/
#include "shhandler.h"
#include "app.h"
#include "doublespinbox.h"
#include "graphicsview.h"

#include <QFont>
#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QtWidgets>

namespace Shapes {

constexpr double StickingDistance = 10;
constexpr double Size = StickingDistance / 2;

void drawPos(QPainter* painter, const QPointF& pt1) {
    QFont font;
    font.setPixelSize(16);
    const QString text = QString(App::settings().inch() ? "  X = %1 in\n"
                                                          "  Y = %2 in\n"
                                                        : "  X = %1 mm\n"
                                                          "  Y = %2 mm\n")
                             .arg(pt1.x() / (App::settings().inch() ? 25.4 : 1.0), 4, 'f', 3, '0')
                             .arg(pt1.y() / (App::settings().inch() ? 25.4 : 1.0), 4, 'f', 3, '0');

    const QRectF textRect = QFontMetricsF(font).boundingRect(QRectF(), Qt::AlignLeft, text);
    const double k = App::graphicsView()->scaleFactor();
    painter->save();
    painter->scale(k, -k);
    int i = 0;
    for (const QString& txt : text.split('\n')) {
        QPainterPath path;
        path.addText(textRect.topLeft() + QPointF(textRect.left(), textRect.height() * 0.25 * ++i), font, txt);
        painter->setPen(QPen(Qt::black, 4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter->setBrush(Qt::NoBrush);
        painter->drawPath(path);
        painter->setPen(Qt::NoPen);
        painter->setBrush(Qt::white);
        painter->drawPath(path);
    }
    painter->restore();
}

Handler::Handler(Shape* shape, Type type)
    : shape(shape)
    , hType_(type) {
    setAcceptHoverEvents(true);
    setFlags(ItemIsMovable);
    setFlags(ItemSendsScenePositionChanges);
    switch (hType_) {
    case Adder:
        setZValue(std::numeric_limits<double>::max() - 2);
        break;
    case Center:
        setZValue(std::numeric_limits<double>::max() - 0);
        break;
    case Corner:
        setZValue(std::numeric_limits<double>::max() - 1);
        break;
    }
    App::graphicsView()->scene()->addItem(this);
    App::shapeHandlers().emplace_back(this);
}

Handler::~Handler() {
    App::shapeHandlers().removeOne(this);
}

QRectF Handler::boundingRect() const { return rect; }

void Handler::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* /*widget*/) {
    static const QColor cc[] {
        Qt::yellow,
        Qt::red,
        Qt::green,
    };

    auto c = cc[hType_];

    if (option->state & QStyle::State_MouseOver)
        drawPos(painter, pos());
    else
        c.setAlpha(100);

    painter->setBrush(c);
    painter->setPen(QPen(Qt::black, 0.0));

    static double scale;
    if (scale != App::graphicsView()->scaleFactor()) {
        double scale = App::graphicsView()->scaleFactor();
        const double k = Size * scale;
        const double s = k * 2;
        rect = { QPointF(-k, -k), QSizeF(s, s) };
    }

    if (!pressed)
        painter->drawEllipse(rect);
}

// void Handler::setPos(const QPointF& pos) {
//     QGraphicsItem::setPos(pos);
//     if (hType_ == Center) {
//         for (size_t i = 1, end = shape->handlers.size(); i < end && i < pt.size(); ++i)
//             shape->handlers[i]->QGraphicsItem::setPos(pt[i] + pos - pt.front());
//     } else if (shape->isFinal) { // прилипание
//         const double k = App::graphicsView()->scaleFactor() * StickingDistance;
//         const bool fl = shape->type() == int(GiType::ShPolyLine) && shape->handlers.size() > 3;
//         for (Handler* h : App::shapeHandlers()) {
//             if (h != this &&                                          //
//                 (h->shape != shape || (fl && h->hType() != Adder)) && //
//                 h->shape->isVisible() &&                              //
//                 QLineF(h->pos(), pos).length() < k                    //
//             ) {
//                 QGraphicsItem::setPos(h->pos());
//                 break;
//             }
//         }
//         shape->updateOtherHandlers(this);
//     }
//     shape->redraw();
// }

Handler::Type Handler::hType() const { return hType_; }

void Handler::setHType(Type value) { hType_ = value; }

// QRectF Handler::rect() const {
//     const double scale = App::graphicsView()->scaleFactor();
//     const double k = Size * scale;
//     const double s = k * 2;
//     return { QPointF(-k, -k), QSizeF(s, s) };
// }

void Handler::contextMenuEvent(QGraphicsSceneContextMenuEvent* event) {
    //    savePos();
    class Dialog : public QDialog {
    public:
        Dialog(Handler* h) {
            setWindowFlags(Qt::Popup);
            setModal(false);
            // clang-format off
            enum PType{ X, Y };
            // clang-format on
            auto dsbx = [h, this](PType type) {
                auto ds = new DoubleSpinBox(this);
                ds->setDecimals(3);
                ds->setRange(-1000, +1000);
                ds->setSuffix(QObject::tr(" mm"));
                ds->setValue(type == X ? h->pos().x() : h->pos().y());
                connect(ds, qOverload<double>(&QDoubleSpinBox::valueChanged),
                    [h, type](auto val) {
                        if (type == X)
                            h->setPos({ val, h->pos().y() });
                        else
                            h->setPos({ h->pos().x(), val });
                        h->shape->updateOtherHandlers(h);
                        h->shape->redraw();
                    });
                return ds;
            };

            auto gl = new QGridLayout(this);
            gl->addWidget(new QLabel("X:", this), 0, 0);
            gl->addWidget(new QLabel("Y:", this), 1, 0);
            gl->addWidget(dsbx(X), 0, 1);
            gl->addWidget(dsbx(Y), 1, 1);
            gl->setContentsMargins(6, 6, 6, 6);
        }
        ~Dialog() = default;
        void leaveEvent(QEvent*) override { reject(); };
    } dialog(this);
    dialog.move(event->screenPos());
    dialog.exec();
}

void Handler::hoverEnterEvent(QGraphicsSceneHoverEvent* event) { shape->hoverEnterEvent(event); }

void Handler::hoverLeaveEvent(QGraphicsSceneHoverEvent* event) { shape->hoverLeaveEvent(event); }

// void Handler::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
//     QGraphicsItem::mouseMoveEvent(event);
//     setPos(App::settings().getSnappedPos(pos(), event->modifiers()));
// }

void Handler::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    //    savePos();
    pressed = true;
    setFlag(ItemSendsScenePositionChanges);
    QGraphicsItem::mousePressEvent(event);
}

void Handler::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
    pressed = false;
    QGraphicsItem::mouseReleaseEvent(event);
}

int Handler::type() const { return GiType::ShHandler; }

QVariant Handler::itemChange(GraphicsItemChange change, const QVariant& value) {
    if (change == ItemPositionChange && scene()) {
        qDebug() << __FUNCTION__;
        // value is the new position.
        //        QPointF newPos = value.toPointF();
        //        QRectF rect = scene()->sceneRect();
        //        if (!rect.contains(newPos)) {
        //            // Keep the item inside the scene rect.
        //            newPos.setX(qMin(rect.right(), qMax(newPos.x(), rect.left())));
        //            newPos.setY(qMin(rect.bottom(), qMax(newPos.y(), rect.top())));
        //            return newPos;
        //        }
        //        shape->updateOtherHandlers(this);
        shape->redraw();
    }
    return QGraphicsItem::itemChange(change, value);
}

} // namespace Shapes
