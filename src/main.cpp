#include <Geode/Geode.hpp>
#include <Geode/modify/PlayerObject.hpp>

using namespace geode::prelude;

// Predictive Input Buffer v4 (z logami diagnostycznymi)
//
// Gdy klikniesz w powietrzu (za wczesnie) i puscisz przycisk, mod zapamietuje klikniecie
// na czas okna z ustawien (domyslnie 100 ms) i wykonuje NOWE klikniecie:
//  - w pierwszej klatce na ziemi (szescian, kula, robot, pajak),
//  - w pierwszej klatce, w ktorej dotykasz orba (wszystkie tryby).

namespace {
	template <class T>
	bool touchingRing(T* p) {
		if constexpr (requires { p->m_touchedRing; }) {
			return static_cast<bool>(p->m_touchedRing);
		}
		else if constexpr (requires { p->m_touchedRings; }) {
			return !p->m_touchedRings.empty();
		}
		else {
			return false;
		}
	}

	template <class T>
	constexpr bool ringDetectionAvailable() {
		return requires(T* p) { p->m_touchedRing; } || requires(T* p) { p->m_touchedRings; };
	}

	int g_logFire = 0;
	int g_logPush = 0;
	int g_logRelease = 0;
	int g_logBuffered = 0;
	int g_logExpired = 0;
	int g_logUpdate = 0;
}

class $modify(BufferedPlayer, PlayerObject) {
	struct Fields {
		float clock = 0.f;
		float pressTime = 0.f;
		bool buffered = false;
		bool physHeld = false;
		int autoReleaseSteps = 0;
	};

	static bool bufferOn() {
		return Mod::get()->getSettingValue<bool>("buffer-on") && PlayLayer::get() != nullptr;
	}

	static float windowSeconds() {
		return static_cast<float>(Mod::get()->getSettingValue<int64_t>("window-ms")) / 1000.f;
	}

	bool isGroundMode() {
		return !m_isShip && !m_isBird && !m_isDart && !m_isSwing;
	}

	void fireClick(const char* why) {
		auto f = m_fields.self();
		f->buffered = false;
		PlayerObject::releaseButton(PlayerButton::Jump);
		PlayerObject::pushButton(PlayerButton::Jump);
		f->autoReleaseSteps = 2;
		if (g_logFire < 40) {
			g_logFire++;
			log::info("Predictive: KLIKNIECIE WYKONANE ({})", why);
		}
	}

	void update(float dt) {
		auto f = m_fields.self();
		f->clock += dt;

		// Log kontrolny: czy update w ogole jest wywolywany w poziomie.
		if (g_logUpdate < 3 && PlayLayer::get()) {
			g_logUpdate++;
			log::info("Predictive: update w poziomie, dt={} bufferOn={}", dt, bufferOn());
		}

		if (bufferOn()) {
			if (f->autoReleaseSteps > 0) {
				f->autoReleaseSteps--;
				if (f->autoReleaseSteps == 0 && !f->physHeld) {
					PlayerObject::releaseButton(PlayerButton::Jump);
				}
			}

			if (f->buffered) {
				if (g_logBuffered < 60) {
					g_logBuffered++;
					log::info("Predictive: krok z buforem: onGround={} groundMode={} held={} ring={} wiek={}ms",
						m_isOnGround, isGroundMode(), f->physHeld, touchingRing(this),
						(f->clock - f->pressTime) * 1000.f);
				}

				if (f->clock - f->pressTime > windowSeconds()) {
					f->buffered = false;
					if (g_logExpired < 30) {
						g_logExpired++;
						log::info("Predictive: okno minelo, bufor wygasl");
					}
				}
				else if (m_isOnGround && isGroundMode()) {
					fireClick("ladowanie");
				}
				else if (!f->physHeld && touchingRing(this)) {
					fireClick("orb");
				}
			}
		}

		PlayerObject::update(dt);
	}

	bool pushButton(PlayerButton button) {
		if (button == PlayerButton::Jump) {
			auto f = m_fields.self();
			f->physHeld = true;
			f->autoReleaseSteps = 0;
			bool inLevel = PlayLayer::get() != nullptr;
			if (bufferOn() && !m_isOnGround) {
				f->buffered = true;
				f->pressTime = f->clock;
			}
			else {
				f->buffered = false;
			}
			if (g_logPush < 60 && inLevel) {
				g_logPush++;
				log::info("Predictive: PUSH onGround={} groundMode={} buffered={} bufferOn={}",
					m_isOnGround, isGroundMode(), f->buffered, bufferOn());
			}
		}
		return PlayerObject::pushButton(button);
	}

	bool releaseButton(PlayerButton button) {
		if (button == PlayerButton::Jump) {
			m_fields->physHeld = false;
			if (g_logRelease < 60 && PlayLayer::get()) {
				g_logRelease++;
				log::info("Predictive: RELEASE onGround={} buffered={}", m_isOnGround, m_fields->buffered);
			}
		}
		return PlayerObject::releaseButton(button);
	}

	void resetObject() {
		PlayerObject::resetObject();
		auto f = m_fields.self();
		f->buffered = false;
		f->physHeld = false;
		f->autoReleaseSteps = 0;
		f->clock = 0.f;
		f->pressTime = 0.f;
	}
};

$on_mod(Loaded) {
	log::info("Predictive Input Buffer v4 zaladowany. Wykrywanie orbow: {}",
		ringDetectionAvailable<PlayerObject>() ? "tak" : "NIE (tylko ladowanie)");
}
