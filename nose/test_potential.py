############################TESTS ON POTENTIALS################################
import numpy

#Test whether the normalization of the potential works
def test_normalize_potential():
    from galpy import potential
    #Grab all of the potentials
    pots= [p for p in dir(potential) 
           if ('Potential' in p and not 'plot' in p and not 'RZTo' in p 
               and not 'evaluate' in p)]
    rmpots= ['Potential','MWPotential','MovingObjectPotential',
             'interpRZPotential', 'linearPotential', 'planarAxiPotential',
             'planarPotential', 'verticalPotential']
    for p in rmpots:
        pots.remove(p)
    for p in pots:
        #if not 'NFW' in p: continue #For testing the test
        #Setup instance of potential
        tclass= getattr(potential,p)
        tp= tclass()
        if not hasattr(tp,'normalize'): continue
        tp.normalize(1.)
        try:
            assert((tp.Rforce(1.,0.)+1.)**2. < 10.**-16.)
        except AssertionError:
            raise AssertionError("Normalization of %s potential fails" % p)
        tp.normalize(.5)
        try:
            assert((tp.Rforce(1.,0.)+.5)**2. < 10.**-16.)
        except AssertionError:
            raise AssertionError("Normalization of %s potential fails" % p)


#Test whether the derivative of the potential is minus the force
def test_forceAsDeriv_potential():
    from galpy import potential
    #Grab all of the potentials
    pots= [p for p in dir(potential) 
           if ('Potential' in p and not 'plot' in p and not 'RZTo' in p 
               and not 'evaluate' in p)]
    rmpots= ['Potential','MWPotential','MovingObjectPotential',
             'interpRZPotential', 'linearPotential', 'planarAxiPotential',
             'planarPotential', 'verticalPotential']
    for p in rmpots:
        pots.remove(p)
    Rs= numpy.array([0.5,1.,2.])
    Zs= numpy.array([0.,.125,-.125,0.25,-0.25])
    phis= numpy.array([0.,0.5,-0.5,1.,-1.,
                       numpy.pi,0.5+numpy.pi,
                       1.+numpy.pi])
    #tolerances in log10
    tol= {}
    tol['default']= -8.
    tol['DoubleExponentialDiskPotential']= -6. #these are more difficult
    tol['RazorThinExponentialDiskPotential']= -6.
    for p in pots:
        #if not 'NFW' in p: continue #For testing the test
        #Setup instance of potential
        tclass= getattr(potential,p)
        tp= tclass()
        #raise AttributeError("Do something about potentials without normalize")
        if hasattr(tp,'normalize'): tp.normalize(1.)
        #Set tolerance
        if p in tol.keys(): ttol= tol[p]
        else: ttol= tol['default']
        #Radial force
        for ii in range(len(Rs)):
            for jj in range(len(Zs)):
                dr= 10.**-8.
                newR= Rs[ii]+dr
                dr= newR-Rs[ii] #Representable number
                if isinstance(tp,potential.linearPotential): 
                    mpotderivR= (tp(Rs[ii])-tp(Rs[ii]+dr))/dr
                    tRforce= tp.force(Rs[ii])
                else:
                    mpotderivR= (tp(Rs[ii],Zs[jj])-tp(Rs[ii]+dr,Zs[jj]))/dr
                    tRforce= tp.Rforce(Rs[ii],Zs[jj])
                try:
                    if tRforce**2. < 10.**ttol:
                        assert(mpotderivR**2. < 10.**ttol)
                    else:
                        assert((tRforce-mpotderivR)**2./tRforce**2. < 10.**ttol)
                except AssertionError:
                    raise AssertionError("Calculation of the Radial force as the Radial derivative of the %s potential fails at (R,Z) = (%.3f,%.3f); diff = %e, rel. diff = %e" % (p,Rs[ii],Zs[jj],numpy.fabs(tRforce-mpotderivR), numpy.fabs((tRforce-mpotderivR)/tRforce)))
        #Azimuthal force, if it exists
        if isinstance(tp,potential.linearPotential): continue
        for ii in range(len(Rs)):
            for jj in range(len(phis)):
                dphi= 10.**-8.
                newphi= phis[jj]+dphi
                dphi= newphi-phis[jj] #Representable number
                if isinstance(tp,potential.planarPotential):
                    mpotderivphi= (tp(Rs[ii],phi=phis[jj])-tp(Rs[ii],phi=phis[jj]+dphi))/dphi
                    tphiforce= tp.phiforce(Rs[ii],phi=phis[jj])
                else:
                    mpotderivphi= (tp(Rs[ii],0.05,phi=phis[jj])-tp(Rs[ii],0.05,phi=phis[jj]+dphi))/dphi
                    tphiforce= tp.phiforce(Rs[ii],0.05,phi=phis[jj])
                try:
                    if tphiforce**2. < 10.**ttol:
                        assert(mpotderivphi**2. < 10.**ttol)
                    else:
                        assert((tphiforce-mpotderivphi)**2./tphiforce**2. < 10.**ttol)
                except AssertionError:
                    raise AssertionError("Calculation of the azimuthal force as the azimuthal derivative of the %s potential fails at (R,Z,phi) = (%.3f,%.3f); diff = %e, rel. diff = %e" % (p,Rs[ii],0.05,phis[jj],numpy.fabs(mpotderivphi),numpy.fabs((tphiforce-mpotderivphi)/tphiforce)))
        #Vertical force, if it exists
        if isinstance(tp,potential.planarPotential) \
                or isinstance(tp,potential.linearPotential): continue
        for ii in range(len(Rs)):
            for jj in range(len(Zs)):
                dz= 10.**-8.
                newZ= Zs[jj]+dz
                dz= newZ-Zs[jj] #Representable number
                mpotderivz= (tp(Rs[ii],Zs[jj])-tp(Rs[ii],Zs[jj]+dz))/dz
                tzforce= tp.zforce(Rs[ii],Zs[jj])
                try:
                    if tzforce**2. < 10.**ttol:
                        assert(mpotderivz**2. < 10.**ttol)
                    else:
                        assert((tzforce-mpotderivz)**2./tzforce**2. < 10.**ttol)
                except AssertionError:
                    raise AssertionError("Calculation of the vertical force as the vertical derivative of the %s potential fails at (R,Z) = (%.3f,%.3f); diff = %e, rel. diff = %e" % (p,Rs[ii],Zs[jj],numpy.fabs(mpotderivz),numpy.fabs((tzforce-mpotderivz)/tzforce)))

#Test whether the second derivative of the potential is minus the derivative of the force
def test_2ndDeriv_potential():
    from galpy import potential
    #Grab all of the potentials
    pots= [p for p in dir(potential) 
           if ('Potential' in p and not 'plot' in p and not 'RZTo' in p 
               and not 'evaluate' in p)]
    rmpots= ['Potential','MWPotential','MovingObjectPotential',
             'interpRZPotential', 'linearPotential', 'planarAxiPotential',
             'planarPotential', 'verticalPotential']
    for p in rmpots:
        pots.remove(p)
    Rs= numpy.array([0.5,1.,2.])
    Zs= numpy.array([0.,.125,-.125,0.25,-0.25])
    phis= numpy.array([0.,0.5,-0.5,1.,-1.,
                       numpy.pi,0.5+numpy.pi,
                       1.+numpy.pi])
    #tolerances in log10
    tol= {}
    tol['default']= -8.
    tol['DoubleExponentialDiskPotential']= -3. #these are more difficult
    tol['RazorThinExponentialDiskPotential']= -6.
    for p in pots:
        #if not 'NFW' in p: continue #For testing the test
        #Setup instance of potential
        tclass= getattr(potential,p)
        tp= tclass()
        #raise AttributeError("Do something about potentials without normalize")
        if hasattr(tp,'normalize'): tp.normalize(1.)
        #Set tolerance
        if p in tol.keys(): ttol= tol[p]
        else: ttol= tol['default']
        #2nd radial
        if hasattr(tp,'_R2deriv'):
            for ii in range(len(Rs)):
                for jj in range(len(Zs)):
                    if p == 'RazorThinExponentialDiskPotential' and numpy.fabs(Zs[jj]) > 0.: continue #Not implemented
                    dr= 10.**-8.
                    newR= Rs[ii]+dr
                    dr= newR-Rs[ii] #Representable number
                    if isinstance(tp,potential.linearPotential): 
                        mRforcederivR= (tp.Rforce(Rs[ii])-tp.Rforce(Rs[ii]+dr))/dr
                        tR2deriv= tp.R2deriv(Rs[ii])
                    else:
                        mRforcederivR= (tp.Rforce(Rs[ii],Zs[jj])-tp.Rforce(Rs[ii]+dr,Zs[jj]))/dr
                        tR2deriv= tp.R2deriv(Rs[ii],Zs[jj])
                    try:
                        if tR2deriv**2. < 10.**ttol:
                            assert(mRforcederivR**2. < 10.**ttol)
                        else:
                            assert((tR2deriv-mRforcederivR)**2./tR2deriv**2. < 10.**ttol)
                    except AssertionError:
                        raise AssertionError("Calculation of the second Radial derivative of the potential as the Radial derivative of the %s Radial force fails at (R,Z) = (%.3f,%.3f); diff = %e, rel. diff = %e" % (p,Rs[ii],Zs[jj],numpy.fabs(tR2deriv-mRforcederivR), numpy.fabs((tR2deriv-mRforcederivR)/tR2deriv)))
        #2nd vertical
        if not isinstance(tp,potential.planarPotential) \
                and not isinstance(tp,potential.linearPotential) \
                and hasattr(tp,'_z2deriv'):
            print p, hasattr(tp,'_z2deriv')
            for ii in range(len(Rs)):
                for jj in range(len(Zs)):
                    if p == 'RazorThinExponentialDiskPotential': continue #Not implemented, or badly defined
                    if p == 'TwoPowerSphericalPotential': continue #Not implemented, or badly defined
                    dz= 10.**-8.
                    newz= Zs[jj]+dz
                    dz= newz-Zs[jj] #Representable number
                    mzforcederivz= (tp.zforce(Rs[ii],Zs[jj])-tp.zforce(Rs[ii],Zs[jj]+dz))/dz
                    tz2deriv= tp.z2deriv(Rs[ii],Zs[jj])
                    try:
                        if tz2deriv**2. < 10.**ttol:
                            assert(mzforcederivz**2. < 10.**ttol)
                        else:
                            assert((tz2deriv-mzforcederivz)**2./tz2deriv**2. < 10.**ttol)
                    except AssertionError:
                        raise AssertionError("Calculation of the second vertical derivative of the potential as the vertical derivative of the %s vertical force fails at (R,Z) = (%.3f,%.3f); diff = %e, rel. diff = %e" % (p,Rs[ii],Zs[jj],numpy.fabs(tz2deriv-mzforcederivz), numpy.fabs((tz2deriv-mzforcederivz)/tz2deriv)))
#    raise AssertionError
