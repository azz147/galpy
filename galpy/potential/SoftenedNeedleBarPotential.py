###############################################################################
#   SoftenedNeedleBarPotential.py: class that implements the softened needle
#                                  bar potential from Long & Murali (1992)
###############################################################################
import hashlib

import numpy

from ..util import conversion, coords
from .Potential import Potential


class SoftenedNeedleBarPotential(Potential):
    """Class that implements the softened needle bar potential from `Long & Murali (1992) <http://adsabs.harvard.edu/abs/1992ApJ...397...44L>`__

    .. math::

        \\Phi(x,y,z) = \\frac{\\mathrm{amp}}{2a}\\,\\ln\\left(\\frac{x-a+T_-}{x+a+T_+}\\right)

    where

    .. math::

        T_{\\pm} = \\sqrt{(a\\pm x)^2 + y^2+(b+\\sqrt{z^2+c^2})^2}

    For a prolate bar, set :math:`b` to zero.

    """

    def __init__(
        self,
        amp=1.0,
        a=4.0,
        b=0.0,
        c=1.0,
        normalize=False,
        pa=0.4,
        omegab=1.8,
        ro=None,
        vo=None,
    ):
        """
        NAME:

           __init__

        PURPOSE:

           initialize a softened-needle bar potential

        INPUT:

           amp - amplitude to be applied to the potential (default: 1); can be a Quantity with units of mass

           a= (4.) Bar half-length (can be Quantity)

           b= (1.) Triaxial softening length (can be Quantity)

           c= (1.) Prolate softening length (can be Quantity)

           pa= (0.4) The position angle of the x axis (rad or Quantity)

           omegab= (1.8) Pattern speed (can be Quantity)

           normalize - if True, normalize such that vc(1.,0.)=1., or, if given as a number, such that the force is this fraction of the force necessary to make vc(1.,0.)=1.

           ro=, vo= distance and velocity scales for translation into internal units (default from configuration file)

        OUTPUT:

           (none)

        HISTORY:

           2016-11-02 - Started - Bovy (UofT)

        """
        Potential.__init__(self, amp=amp, ro=ro, vo=vo, amp_units="mass")
        a = conversion.parse_length(a, ro=self._ro)
        b = conversion.parse_length(b, ro=self._ro)
        c = conversion.parse_length(c, ro=self._ro)
        pa = conversion.parse_angle(pa)
        omegab = conversion.parse_frequency(omegab, ro=self._ro, vo=self._vo)
        self._a = a
        self._b = b
        self._c2 = c**2.0
        self._pa = pa
        self._omegab = omegab
        self._force_hash = None
        self.hasC = True
        self.hasC_dxdv = False
        if normalize or (
            isinstance(normalize, (int, float)) and not isinstance(normalize, bool)
        ):  # pragma: no cover
            self.normalize(normalize)
        self.isNonAxi = True
        return None

    def _evaluate(self, R, z, phi=0.0, t=0.0):
        """
        NAME:
           _evaluate
        PURPOSE:
           evaluate the potential at R,z
        INPUT:
           R - Galactocentric cylindrical radius
           z - vertical height
           phi - azimuth
           t - time
        OUTPUT:
           Phi(R,z)
        HISTORY:
           2016-11-02 - Started - Bovy (UofT)
        """
        x, y, z = self._compute_xyz(R, phi, z, t)
        Tp, Tm = self._compute_TpTm(x, y, z)
        return numpy.log((x - self._a + Tm) / (x + self._a + Tp)) / 2.0 / self._a

    def _Rforce(self, R, z, phi=0.0, t=0.0):
        """
        NAME:
           _Rforce
        PURPOSE:
           evaluate the radial force for this potential
        INPUT:
           R - Galactocentric cylindrical radius
           z - vertical height
           phi - azimuth
           t - time
        OUTPUT:
           the radial force
        HISTORY:
           2016-11-02 - Written - Bovy (UofT)
        """
        self._compute_xyzforces(R, z, phi, t)
        return numpy.cos(phi) * self._cached_Fx + numpy.sin(phi) * self._cached_Fy

    def _phitorque(self, R, z, phi=0.0, t=0.0):
        """
        NAME:
           _phitorque
        PURPOSE:
           evaluate the azimuthal torque for this potential
        INPUT:
           R - Galactocentric cylindrical radius
           z - vertical height
           phi - azimuth
           t - time
        OUTPUT:
           the azimuthal torque
        HISTORY:
           2016-11-02 - Written - Bovy (UofT)
        """
        self._compute_xyzforces(R, z, phi, t)
        return R * (
            -numpy.sin(phi) * self._cached_Fx + numpy.cos(phi) * self._cached_Fy
        )

    def _zforce(self, R, z, phi=0.0, t=0.0):
        """
        NAME:
           _zforce
        PURPOSE:
           evaluate the vertical force for this potential
        INPUT:
           R - Galactocentric cylindrical radius
           z - vertical height
           phi - azimuth
           t - time
        OUTPUT:
           the vertical force
        HISTORY:
           2016-11-02 - Written - Bovy (UofT)
        """
        self._compute_xyzforces(R, z, phi, t)
        return self._cached_Fz

    def OmegaP(self):
        """
        NAME:
           OmegaP
        PURPOSE:
           return the pattern speed
        INPUT:
           (none)
        OUTPUT:
           pattern speed
        HISTORY:
           2016-11-02 - Written - Bovy (UofT)
        """
        return self._omegab

    def _compute_xyz(self, R, phi, z, t):
        return coords.cyl_to_rect(R, phi - self._pa - self._omegab * t, z)

    def _compute_TpTm(self, x, y, z):
        secondpart = y**2.0 + (self._b + numpy.sqrt(self._c2 + z**2.0)) ** 2.0
        return (
            numpy.sqrt((self._a + x) ** 2.0 + secondpart),
            numpy.sqrt((self._a - x) ** 2.0 + secondpart),
        )

    def _compute_xyzforces(self, R, z, phi, t):
        # Compute all rectangular forces
        new_hash = hashlib.md5(numpy.array([R, phi, z, t])).hexdigest()
        if new_hash != self._force_hash:
            x, y, z = self._compute_xyz(R, phi, z, t)
            Tp, Tm = self._compute_TpTm(x, y, z)
            Fx = self._xforce_xyz(x, y, z, Tp, Tm)
            Fy = self._yforce_xyz(x, y, z, Tp, Tm)
            Fz = self._zforce_xyz(x, y, z, Tp, Tm)
            self._force_hash = new_hash
            tp = self._pa + self._omegab * t
            cp, sp = numpy.cos(tp), numpy.sin(tp)
            self._cached_Fx = cp * Fx - sp * Fy
            self._cached_Fy = sp * Fx + cp * Fy
            self._cached_Fz = Fz

    def _xforce_xyz(self, x, y, z, Tp, Tm):
        return -2.0 * x / Tp / Tm / (Tp + Tm)

    def _yforce_xyz(self, x, y, z, Tp, Tm):
        return (
            -y
            / 2.0
            / Tp
            / Tm
            * (Tp + Tm - 4.0 * x**2.0 / (Tp + Tm))
            / (y**2.0 + (self._b + numpy.sqrt(z**2.0 + self._c2)) ** 2.0)
        )

    def _zforce_xyz(self, x, y, z, Tp, Tm):
        zc = numpy.sqrt(z**2.0 + self._c2)
        return self._yforce_xyz(x, y, z, Tp, Tm) * z / y * (self._b + zc) / zc

    def _dens(self, R, z, phi=0.0, t=0.0):
        """
        NAME:
           _dens
        PURPOSE:
           evaluate the density for this potential
        INPUT:
           R - Galactocentric cylindrical radius
           z - vertical height
           phi - azimuth
           t - time
        OUTPUT:
           the density
        HISTORY:
           2016-11-04 - Written - Bovy (UofT/CCA)
        """
        x, y, z = self._compute_xyz(R, phi, z, t)
        zc = numpy.sqrt(z**2.0 + self._c2)
        bzc2 = (self._b + zc) ** 2.0
        bigA = self._b * y**2.0 + (self._b + 3.0 * zc) * bzc2
        bigC = y**2.0 + bzc2
        return (
            self._c2
            / 24.0
            / numpy.pi
            / self._a
            / bigC**2.0
            / zc**3.0
            * (
                (x + self._a)
                * (
                    3.0 * bigA * bigC
                    + (2.0 * bigA + self._b * bigC) * (x + self._a) ** 2.0
                )
                / (bigC + (x + self._a) ** 2.0) ** 1.5
                - (x - self._a)
                * (
                    3.0 * bigA * bigC
                    + (2.0 * bigA + self._b * bigC) * (x - self._a) ** 2.0
                )
                / (bigC + (x - self._a) ** 2.0) ** 1.5
            )
        )
