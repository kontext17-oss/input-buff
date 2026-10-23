#include <Geode/Geode.hpp>
#include <Geode/modify/PlayerObject.hpp>

using namespace geode::prelude;

// Predictive Input Buffer v3
//
// Gdy klikniesz w powietrzu (za wczesnie) i puscisz przycisk, mod zapamietuje klikniecie
// na czas okna z ustawien (domyslnie 100 ms) i wykonuje NOWE klikniecie:
//  - w pierwszej klatce na ziemi (sześcian, kula, robot, pająk),
//  - w pierwszej klatce, w ktorej dotykasz orba (wszystkie tryby).

namespace {
	// Kompiluje sie niezaleznie od tego, jak pole od orba nazywa sie w tej wersji bindingow.
	template <class T>
	bool touchingRing(T* p) {
		if constexpr (requires { p->m_touchedRing; }) {
			return p->m_touchedRing != nullptr;
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

	int g_logged = 0;
}

class $modify(BufferedPlayer, PlayerObject) {
	struct Fields {
		float clock = 0.f;        // wlasny zegar (sekundy)
		float pressTime = 0.f;    // kiedy zapamietano klikniecie
		bool buffered = false;    // czekamy na ladowanie / orb
		bool physHeld = false;    // czy przycisk jest fizycznie trzymany
		int autoReleaseSteps = 0; // po ilu krokach puscic nasze klikniecie
	};

	static bool bufferOn() {
		return Mod::get()->getSettingValue<bool>("buffer-on") && PlayLayer::get() != nullptr;
	}

	static float windowSeconds() {
		return static_cast<float>(Mod::get()->getSettingValue<int64_t>("window-ms")) / 1000.f;
	}

	// Tryby, w ktorych klikniecie na ziemi cos robi (skok / zmiana grawitacji).
	bool isGroundMode() {
		return !m_isShip && !m_isBird && !m_isDart && !m_isSwing;
	}

	void fireClick(const char* why) {
		auto f = m_fields.self();
		f->buffered = false;
		PlayerObject::releaseButton(PlayerButton::Jump);
		PlayerObject::pushButton(PlayerButton::Jump);
		f->autoReleaseSteps = 2;
		if (g_logged < 30) {
			g_logged++;
			log::info("Predictive Input Buffer: klikniecie wykonane ({})", why);
		}
	}

	void update(float dt) {
		auto f = m_fields.self();
		f->clock += dt;

		if (bufferOn()) {
			// Puszczamy nasze automatyczne klikniecie po krotkiej chwili.
			if (f->autoReleaseSteps > 0) {
				f->autoReleaseSteps--;
				if (f->autoReleaseSteps == 0 && !f->physHeld) {
					PlayerObject::releaseButton(PlayerButton::Jump);
				}
			}

			if (f->buffered) {
				if (f->clock - f->pressTime > windowSeconds()) {
					f->buffered = false; // okno minelo
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
			if (bufferOn() && !m_isOnGround) {
				f->buffered = true;
				f->pressTime = f->clock;
			}
			else {
				f->buffered = false;
			}
		}
		return PlayerObject::pushButton(button);
	}

	bool releaseButton(PlayerButton button) {
		if (button == PlayerButton::Jump) {
			m_fields->physHeld = false;
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
	log::info("Predictive Input Buffer v3 zaladowany. Wykrywanie orbow: {}",
		ringDetectionAvailable<PlayerObject>() ? "tak" : "NIE (tylko ladowanie)");
}
