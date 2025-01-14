/*
Project Name : OpenMEEG

© INRIA and ENPC (contributors: Geoffray ADDE, Maureen CLERC, Alexandre
GRAMFORT, Renaud KERIVEN, Jan KYBIC, Perrine LANDREAU, Théodore PAPADOPOULO,
Emmanuel OLIVI
Maureen.Clerc.AT.inria.fr, keriven.AT.certis.enpc.fr,
kybic.AT.fel.cvut.cz, papadop.AT.inria.fr)

The OpenMEEG software is a C++ package for solving the forward/inverse
problems of electroencephalography and magnetoencephalography.

This software is governed by the CeCILL-B license under French law and
abiding by the rules of distribution of free software.  You can  use,
modify and/ or redistribute the software under the terms of the CeCILL-B
license as circulated by CEA, CNRS and INRIA at the following URL
"http://www.cecill.info".

As a counterpart to the access to the source code and  rights to copy,
modify and redistribute granted by the license, users are provided only
with a limited warranty  and the software's authors,  the holders of the
economic rights,  and the successive licensors  have only  limited
liability.

In this respect, the user's attention is drawn to the risks associated
with loading,  using,  modifying and/or developing or reproducing the
software by the user in light of its specific status of free software,
that may mean  that it is complicated to manipulate,  and  that  also
therefore means  that it is reserved for developers  and  experienced
professionals having in-depth computer knowledge. Users are therefore
encouraged to load and test the software's suitability as regards their
requirements in conditions enabling the security of their systems and/or
data to be ensured and,  more generally, to use and operate it in the
same conditions as regards security.

The fact that you are presently reading this means that you have had
knowledge of the CeCILL-B license and that you accept its terms.
*/

#pragma once

#include <iostream>
#include <cstdlib>
#include <string>

#include <vector.h>
#include <linop.h>

namespace OpenMEEG {

    class Matrix;

    class OPENMEEGMATHS_EXPORT SymMatrix : public LinOp {

        friend class Vector;

        LinOpValue value;

    public:

        SymMatrix(): LinOp(0,0,SYMMETRIC,2),value() {}

        SymMatrix(const char* fname): LinOp(0,0,SYMMETRIC,2),value() { this->load(fname); }
        SymMatrix(Dimension N): LinOp(N,N,SYMMETRIC,2),value(size()) { }
        SymMatrix(Dimension M,Dimension N): LinOp(N,N,SYMMETRIC,2),value(size()) { om_assert(N==M); }
        SymMatrix(const SymMatrix& S,const DeepCopy): LinOp(S.nlin(),S.nlin(),SYMMETRIC,2),value(S.size(),S.data()) { }

        explicit SymMatrix(const Vector& v);
        explicit SymMatrix(const Matrix& A);

        size_t size() const { return nlin()*(nlin()+1)/2; };
        void info() const ;

        Dimension  ncol() const { return nlin(); } // SymMatrix only need num_lines
        Dimension& ncol()       { return nlin(); }

        void alloc_data() { value = LinOpValue(size()); }
        void reference_data(const double* array) { value = LinOpValue(size(),array); }

        bool empty() const { return value.empty(); }
        void set(double x) ;
        double* data() const { return value.get(); }

        double  operator()(const Index i,const Index j) const {
            om_assert(i<nlin());
            om_assert(j<nlin());
            return data()[(i<=j) ? i+j*(j+1)/2 : j+i*(i+1)/2];
        }

        double& operator()(const Index i,const Index j) {
            om_assert(i<nlin());
            om_assert(j<nlin());
            return data()[(i<=j) ? i+j*(j+1)/2 : j+i*(i+1)/2];
        }

        Matrix    operator()(const Index i_start,const Index i_end,const Index j_start,const Index j_end) const;
        Matrix    submat(const Index istart,const Index isize,const Index jstart,const Index jsize) const;
        SymMatrix submat(const Index istart,const Index iend) const;
        Vector    getlin(const Index i) const;
        void      setlin(const Index i,const Vector& v);
        Vector    solveLin(const Vector& B) const;
        void      solveLin(Vector* B,const int nbvect);
        Matrix    solveLin(Matrix& B) const;

        const SymMatrix& operator=(const double d);

        SymMatrix operator+(const SymMatrix& B) const;
        SymMatrix operator-(const SymMatrix& B) const;
        Matrix    operator*(const SymMatrix& B) const;
        Matrix    operator*(const Matrix& B) const;
        Vector    operator*(const Vector& v) const;
        SymMatrix operator*(const double x) const;
        SymMatrix operator/(const double x) const { return (*this)*(1/x); }

        void operator +=(const SymMatrix& B);
        void operator -=(const SymMatrix& B);
        void operator *=(const double x);
        void operator /=(const double x) { (*this)*=(1/x); }

        SymMatrix inverse() const;
        void invert();
        SymMatrix posdefinverse() const;
        double det();
        // void eigen(Matrix& Z,Vector& D);

        void save(const char* filename) const;
        void load(const char* filename);

        void save(const std::string& s) const { save(s.c_str()); }
        void load(const std::string& s)       { load(s.c_str()); }

        friend class Matrix;
    };

    // Returns the solution of (this)*X = B

    inline Vector SymMatrix::solveLin(const Vector& B) const {
        SymMatrix invA(*this,DEEP_COPY);
        Vector X(B,DEEP_COPY);

    #ifdef HAVE_LAPACK
        // Bunch Kaufman factorization
        BLAS_INT* pivots=new BLAS_INT[nlin()];
        int Info = 0;
        DSPTRF('U',sizet_to_int(invA.nlin()),invA.data(),pivots,Info);
        // Inverse
        DSPTRS('U',sizet_to_int(invA.nlin()),1,invA.data(),pivots,X.data(),sizet_to_int(invA.nlin()),Info);

        om_assert(Info==0);
        delete[] pivots;
    #else
        std::cout << "solveLin not defined" << std::endl;
    #endif
        return X;
    }

    // stores in B the solution of (this)*X = B, where B is a set of nbvect vector

    inline void SymMatrix::solveLin(Vector* B,const int nbvect) {
        SymMatrix invA(*this,DEEP_COPY);

    #ifdef HAVE_LAPACK
        // Bunch Kaufman Factorization
        BLAS_INT *pivots=new BLAS_INT[nlin()];
        int Info = 0;
        //char *uplo="U";
        DSPTRF('U',sizet_to_int(invA.nlin()),invA.data(),pivots,Info);
        // Inverse
        for(int i = 0; i < nbvect; i++)
            DSPTRS('U',sizet_to_int(invA.nlin()),1,invA.data(),pivots,B[i].data(),sizet_to_int(invA.nlin()),Info);

        om_assert(Info==0);
        delete[] pivots;
    #else
        std::cout << "solveLin not defined" << std::endl;
    #endif
    }

    inline void SymMatrix::operator+=(const SymMatrix& B) {
        om_assert(nlin()==B.nlin());
    #ifdef HAVE_BLAS
        BLAS(daxpy,DAXPY)(sizet_to_int(nlin()*(nlin()+1)/2), 1.0, B.data(), 1, data() , 1);
    #else
        const size_t sz = size();
        for (size_t i=0; i<sz; ++i)
            data()[i] += B.data()[i];
    #endif
    }

    inline void SymMatrix::operator-=(const SymMatrix& B) {
        om_assert(nlin()==B.nlin());
    #ifdef HAVE_BLAS
        BLAS(daxpy,DAXPY)(sizet_to_int(nlin()*(nlin()+1)/2), -1.0, B.data(), 1, data() , 1);
    #else
        const size_t sz = size();
        for (size_t i=0; i<sz; ++i)
            data()[i] -= B.data()[i];
    #endif
    }

    inline SymMatrix SymMatrix::posdefinverse() const {
        // supposes (*this) is definite positive
        SymMatrix invA(*this,DEEP_COPY);
    #ifdef HAVE_LAPACK
        // U'U factorization then inverse
        int Info = 0;
        DPPTRF('U', sizet_to_int(nlin()),invA.data(),Info);
        DPPTRI('U', sizet_to_int(nlin()),invA.data(),Info);
        om_assert(Info==0);
    #else
        std::cerr << "Positive definite inverse not defined" << std::endl;
    #endif
        return invA;
    }

    inline double SymMatrix::det() {
        SymMatrix invA(*this,DEEP_COPY);
        double d = 1.0;
    #ifdef HAVE_LAPACK
        // Bunch Kaufmqn
        BLAS_INT *pivots=new BLAS_INT[nlin()];
        int Info = 0;
        // TUDUtTt
        DSPTRF('U', sizet_to_int(invA.nlin()), invA.data(), pivots,Info);
        if (Info<0)
            std::cout << "Big problem in det (DSPTRF)" << std::endl;
        for (size_t i = 0; i< nlin(); i++){
            if (pivots[i] >= 0) {
                d *= invA(i,i);
            } else { // pivots[i] < 0
                if (i < nlin()-1 && pivots[i] == pivots[i+1]) {
                    d *= (invA(i,i)*invA(i+1,i+1)-invA(i,i+1)*invA(i+1,i));
                    i++;
                } else {
                    std::cout << "Big problem in det" << std::endl;
                }
            }
        }
        delete[] pivots;
    #else
        std::cerr << "Determinant not defined without LAPACK" << std::endl;
        exit(1);
    #endif
        return(d);
    }

    // inline void SymMatrix::eigen(Matrix& Z,Vector& D ){
    //     // performs the complete eigen-decomposition.
    //     //  (*this) = Z.D.Z'
    //     // -> eigenvector are columns of the Matrix Z.
    //     // (*this).Z[:,i] = D[i].Z[:,i]
    // #ifdef HAVE_LAPACK
    //     SymMatrix symtemp(*this,DEEP_COPY);
    //     D = Vector(nlin());
    //     Z = Matrix(nlin(),nlin());
    // 
    //     int info;
    //     double lworkd;
    //     int lwork;
    //     int liwork;
    // 
    //     DSPEVD('V','U',sizet_to_int(nlin()),symtemp.data(),D.data(),Z.data(),sizet_to_int(nlin()),&lworkd,-1,&liwork,-1,info);
    //     lwork = (int) lworkd;
    //     double * work = new double[lwork];
    //     BLAS_INT *iwork = new BLAS_INT[liwork];
    //     DSPEVD('V','U',sizet_to_int(nlin()),symtemp.data(),D.data(),Z.data(),sizet_to_int(nlin()),work,lwork,iwork,liwork,info);
    // 
    //     delete[] work;
    //     delete[] iwork;
    // #endif
    // }

    inline SymMatrix SymMatrix::operator+(const SymMatrix& B) const {
        om_assert(nlin()==B.nlin());
        SymMatrix C(*this,DEEP_COPY);
        C += B;
        return C;
    }

    inline SymMatrix SymMatrix::operator-(const SymMatrix& B) const {
        om_assert(nlin()==B.nlin());
        SymMatrix C(*this,DEEP_COPY);
        C -= B;
        return C;
    }

    inline SymMatrix SymMatrix::inverse() const {
    #ifdef HAVE_LAPACK
        SymMatrix invA(*this, DEEP_COPY);
        // LU
        BLAS_INT* pivots = new BLAS_INT[nlin()];
        int Info = 0;
        const BLAS_INT M = sizet_to_int(nlin());
        DSPTRF('U',M,invA.data(),pivots,Info);
        // Inverse
        double* work = new double[nlin()*64];
        DSPTRI('U',M,invA.data(),pivots,work,Info);

        om_assert(Info==0);
        delete[] pivots;
        delete[] work;
        return invA;
    #else
        std::cerr << "!!!!! Inverse not implemented !!!!!" << std::endl;
        exit(1);
    #endif
    }

    inline void SymMatrix::invert() {
    #ifdef HAVE_LAPACK
        // LU
        BLAS_INT* pivots = new BLAS_INT[nlin()];
        int Info = 0;
        const BLAS_INT M = sizet_to_int(nlin());
        DSPTRF('U',M,data(),pivots,Info);
        // Inverse
        double* work = new double[nlin()*64];
        DSPTRI('U',M,data(),pivots,work,Info);

        om_assert(Info==0);
        delete[] pivots;
        delete[] work;
        return;
    #else
        std::cerr << "!!!!! Inverse not implemented !!!!!" << std::endl;
        exit(1);
    #endif
    }

    inline Vector SymMatrix::operator*(const Vector& v) const {
        om_assert(nlin()==v.size());
        Vector y(nlin());
    #ifdef HAVE_BLAS
        const BLAS_INT M = sizet_to_int(nlin());
        DSPMV(CblasUpper,M,1.0,data(),v.data(),1,0.0,y.data(),1);
    #else
        for (Index i=0; i<nlin(); ++i) {
            y(i)=0;
            for (Index j=0; j<nlin(); ++j)
                y(i)+=(*this)(i,j)*v(j);
        }
    #endif
        return y;
    }

    inline Vector SymMatrix::getlin(const Index i) const {
        om_assert(i<nlin());
        Vector v(ncol());
        for (Index j=0; j<ncol(); ++j)
            v(j) = (*this)(i,j);
        return v;
    }

    inline void SymMatrix::setlin(const Index i,const Vector& v) {
        om_assert(v.size()==nlin());
        om_assert(i<nlin());
        for (Index j=0; j<ncol(); ++j)
            (*this)(i,j) = v(j);
    }
}
