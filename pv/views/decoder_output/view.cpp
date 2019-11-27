/*
 * This file is part of the PulseView project.
 *
 * Copyright (C) 2019 Soeren Apel <soeren@apelpie.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include <libsigrokdecode/libsigrokdecode.h>

#include <QMenu>
#include <QToolBar>
#include <QVBoxLayout>

#include "view.hpp"

#include "pv/session.hpp"
#include "pv/util.hpp"

using pv::util::TimeUnit;
using pv::util::Timestamp;

using std::shared_ptr;

namespace pv {
namespace views {
namespace decoder_output {

View::View(Session &session, bool is_main_view, QMainWindow *parent) :
	ViewBase(session, is_main_view, parent)

	// Note: Place defaults in View::reset_view_state(), not here
{
	QVBoxLayout *root_layout = new QVBoxLayout(this);
	root_layout->setContentsMargins(0, 0, 0, 0);

	QToolBar* tool_bar = new QToolBar();
	tool_bar->setContextMenuPolicy(Qt::PreventContextMenu);

	parent->addToolBar(tool_bar);

	reset_view_state();
}

View::~View()
{
}

ViewType View::get_type() const
{
	return ViewTypeDecoderOutput;
}

void View::reset_view_state()
{
	ViewBase::reset_view_state();
}

void View::clear_signals()
{
	ViewBase::clear_signalbases();
}

void View::clear_decode_signals()
{
}

void View::add_decode_signal(shared_ptr<data::DecodeSignal> signal)
{
	connect(signal.get(), SIGNAL(name_changed(const QString&)),
		this, SLOT(on_signal_name_changed()));
}

void View::remove_decode_signal(shared_ptr<data::DecodeSignal> signal)
{
	(void)signal;
}

void View::save_settings(QSettings &settings) const
{
	(void)settings;
}

void View::restore_settings(QSettings &settings)
{
	// Note: It is assumed that this function is only called once,
	// immediately after restoring a previous session.
	(void)settings;
}

void View::on_signal_name_changed()
{
}

} // namespace decoder_output
} // namespace views
} // namespace pv
